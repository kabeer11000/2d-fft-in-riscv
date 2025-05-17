// Compile with:
// riscv64-linux-gnu-g++ -O3 -march=rv64gcv -mabi=lp64d -std=c++17 \
//    -I. http_fft.cpp -o http_fft -lpthread

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <riscv_vector.h>
#include "lodepng.h"

// Simple single‑threaded HTTP server (socket+fork)
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

static const int PORT = 8080;

// 1D Cooley‑Tuk FFT on real input, in‑place, length N=power‑of‑two
void fft1d(std::vector<float>& re, std::vector<float>& im, size_t N) {
    // bit‑reversal
    for (size_t i = 1, j = 0; i < N; i++) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) std::swap(re[i], re[j]), std::swap(im[i], im[j]);
    }
    // Cooley‑Tuk
    for (size_t len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * M_PI / len;
        float wre = cosf(ang), wim = sinf(ang);
        for (size_t i = 0; i < N; i += len) {
            float u_re = 1.0f, u_im = 0.0f;
            for (size_t k = 0; k < (len>>1); k++) {
                size_t idx = i + k, idy = i + k + (len>>1);
                float t_re = u_re * re[idy] - u_im * im[idy];
                float t_im = u_re * im[idy] + u_im * re[idy];
                re[idy] = re[idx] - t_re;
                im[idy] = im[idx] - t_im;
                re[idx] += t_re;
                im[idx] += t_im;
                // update u *= w
                float nxt_re = u_re * wre - u_im * wim;
                u_im = u_re * wim + u_im * wre;
                u_re = nxt_re;
            }
        }
    }
}

// 2D FFT: rows then columns
void fft2d(std::vector<float>& re, std::vector<float>& im, size_t W, size_t H) {
    std::vector<float> tre(std::max(W,H)), tim(std::max(W,H));
    // rows
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            tre[x] = re[y*W + x];
            tim[x] = im[y*W + x];
        }
        fft1d(tre, tim, W);
        for (size_t x = 0; x < W; x++) {
            re[y*W + x] = tre[x];
            im[y*W + x] = tim[x];
        }
    }
    // cols
    for (size_t x = 0; x < W; x++) {
        for (size_t y = 0; y < H; y++) {
            tre[y] = re[y*W + x];
            tim[y] = im[y*W + x];
        }
        fft1d(tre, tim, H);
        for (size_t y = 0; y < H; y++) {
            re[y*W + x] = tre[y];
            im[y*W + x] = tim[y];
        }
    }
}

// Zero‑out small coefficients (simple “compression”)
void quantize(std::vector<float>& re, std::vector<float>& im, float thresh) {
    for (size_t i = 0; i < re.size(); i++) {
        if (fabsf(re[i]) < thresh) re[i] = 0;
        if (fabsf(im[i]) < thresh) im[i] = 0;
    }
}

// Fetch URL into memory (very naive HTTP/1.0 GET)
std::vector<unsigned char> fetch_url(const char* host, const char* path) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in srv = {};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(80);
    inet_pton(AF_INET, host, &srv.sin_addr);
    connect(sock, (sockaddr*)&srv, sizeof(srv));
    char req[512];
    snprintf(req, sizeof(req),
      "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    write(sock, req, strlen(req));
    // skip headers
    char buf[1024];
    int len; bool hdr = true;
    std::vector<unsigned char> img;
    while ((len = read(sock, buf, sizeof(buf))) > 0) {
        if (hdr) {
            const char* p = strstr(buf,"\r\n\r\n");
            if (p) { hdr = false; int off = (p+4)-buf;
                img.insert(img.end(), buf+off, buf+len);
            }
        } else {
            img.insert(img.end(), buf, buf+len);
        }
    }
    close(sock);
    return img;
}

// Handle one HTTP request
void handle_client(int client) {
    char line[1024];
    read(client, line, sizeof(line));
    // parse: GET /?url=http://1.2.3.4/img.png
    char url[512];
    if (sscanf(line, "GET /?url=http://%511[^ ] ", url) != 1) {
        close(client);
        return;
    }
    // split host/path
    char *slash = strchr(url,'/');
    *slash = 0;
    char *host = url, *path = slash;
    auto png = fetch_url(host, path);
    // decode
    unsigned W, H;
    std::vector<unsigned char> in, out;
    lodepng::decode(in, W, H, png);
    // grayscale
    std::vector<float> re(W*H), im(W*H, 0.0f);
    for (size_t i=0;i<W*H;i++)
      re[i] = (in[4*i]+in[4*i+1]+in[4*i+2])/3.0f;
    // FFT + quantize + inverse FFT
    fft2d(re, im, W, H);
    quantize(re, im, 10.0f);
    // inverse: reuse fft2d (≈ its own inverse up to scaling)
    fft2d(re, im, W, H);
    // build out RGBA
    out.resize(4*W*H);
    for (size_t i=0;i<W*H;i++){
      unsigned v = (unsigned)fminf(fmaxf(re[i]/(W*H), 0.0f), 255.0f);
      out[4*i+0]=out[4*i+1]=out[4*i+2]=v;
      out[4*i+3]=255;
    }
    // re‑encode
    std::vector<unsigned char> png_out;
    lodepng::encode(png_out, out, W, H);
    // reply
    dprintf(client,
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: image/png\r\n"
      "Content-Length: %zu\r\n\r\n",
      png_out.size());
    write(client, png_out.data(), png_out.size());
    close(client);
}

int main() {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt=1; setsockopt(srv,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    sockaddr_in addr = {};
    addr.sin_family=AF_INET; addr.sin_port=htons(PORT);
    addr.sin_addr.s_addr=INADDR_ANY;
    bind(srv,(sockaddr*)&addr,sizeof(addr));
    listen(srv,4);

    while (1) {
        int cli = accept(srv,0,0);
        std::thread(handle_client, cli).detach();
    }
    return 0;
}
// Compile with:
// riscv64-linux-gnu-g++ -O3 -march=rv64gcv -mabi=lp64d -std=c++17 \
//    -I. http_fft.cpp -o http_fft -lpthread

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <riscv_vector.h>
#include "lodepng.h"

// Simple single‑threaded HTTP server (socket+fork)
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

static const int PORT = 8080;

// 1D Cooley‑Tuk FFT on real input, in‑place, length N=power‑of‑two
void fft1d(std::vector<float>& re, std::vector<float>& im, size_t N) {
    // bit‑reversal
    for (size_t i = 1, j = 0; i < N; i++) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) std::swap(re[i], re[j]), std::swap(im[i], im[j]);
    }
    // Cooley‑Tuk
    for (size_t len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * M_PI / len;
        float wre = cosf(ang), wim = sinf(ang);
        for (size_t i = 0; i < N; i += len) {
            float u_re = 1.0f, u_im = 0.0f;
            for (size_t k = 0; k < (len>>1); k++) {
                size_t idx = i + k, idy = i + k + (len>>1);
                float t_re = u_re * re[idy] - u_im * im[idy];
                float t_im = u_re * im[idy] + u_im * re[idy];
                re[idy] = re[idx] - t_re;
                im[idy] = im[idx] - t_im;
                re[idx] += t_re;
                im[idx] += t_im;
                // update u *= w
                float nxt_re = u_re * wre - u_im * wim;
                u_im = u_re * wim + u_im * wre;
                u_re = nxt_re;
            }
        }
    }
}

// 2D FFT: rows then columns
void fft2d(std::vector<float>& re, std::vector<float>& im, size_t W, size_t H) {
    std::vector<float> tre(std::max(W,H)), tim(std::max(W,H));
    // rows
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            tre[x] = re[y*W + x];
            tim[x] = im[y*W + x];
        }
        fft1d(tre, tim, W);
        for (size_t x = 0; x < W; x++) {
            re[y*W + x] = tre[x];
            im[y*W + x] = tim[x];
        }
    }
    // cols
    for (size_t x = 0; x < W; x++) {
        for (size_t y = 0; y < H; y++) {
            tre[y] = re[y*W + x];
            tim[y] = im[y*W + x];
        }
        fft1d(tre, tim, H);
        for (size_t y = 0; y < H; y++) {
            re[y*W + x] = tre[y];
            im[y*W + x] = tim[y];
        }
    }
}

// Zero‑out small coefficients (simple “compression”)
void quantize(std::vector<float>& re, std::vector<float>& im, float thresh) {
    for (size_t i = 0; i < re.size(); i++) {
        if (fabsf(re[i]) < thresh) re[i] = 0;
        if (fabsf(im[i]) < thresh) im[i] = 0;
    }
}

// Fetch URL into memory (very naive HTTP/1.0 GET)
std::vector<unsigned char> fetch_url(const char* host, const char* path) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in srv = {};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(80);
    inet_pton(AF_INET, host, &srv.sin_addr);
    connect(sock, (sockaddr*)&srv, sizeof(srv));
    char req[512];
    snprintf(req, sizeof(req),
      "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    write(sock, req, strlen(req));
    // skip headers
    char buf[1024];
    int len; bool hdr = true;
    std::vector<unsigned char> img;
    while ((len = read(sock, buf, sizeof(buf))) > 0) {
        if (hdr) {
            const char* p = strstr(buf,"\r\n\r\n");
            if (p) { hdr = false; int off = (p+4)-buf;
                img.insert(img.end(), buf+off, buf+len);
            }
        } else {
            img.insert(img.end(), buf, buf+len);
        }
    }
    close(sock);
    return img;
}

// Handle one HTTP request
void handle_client(int client) {
    char line[1024];
    read(client, line, sizeof(line));
    // parse: GET /?url=http://1.2.3.4/img.png
    char url[512];
    if (sscanf(line, "GET /?url=http://%511[^ ] ", url) != 1) {
        close(client);
        return;
    }
    // split host/path
    char *slash = strchr(url,'/');
    *slash = 0;
    char *host = url, *path = slash;
    auto png = fetch_url(host, path);
    // decode
    unsigned W, H;
    std::vector<unsigned char> in, out;
    lodepng::decode(in, W, H, png);
    // grayscale
    std::vector<float> re(W*H), im(W*H, 0.0f);
    for (size_t i=0;i<W*H;i++)
      re[i] = (in[4*i]+in[4*i+1]+in[4*i+2])/3.0f;
    // FFT + quantize + inverse FFT
    fft2d(re, im, W, H);
    quantize(re, im, 10.0f);
    // inverse: reuse fft2d (≈ its own inverse up to scaling)
    fft2d(re, im, W, H);
    // build out RGBA
    out.resize(4*W*H);
    for (size_t i=0;i<W*H;i++){
      unsigned v = (unsigned)fminf(fmaxf(re[i]/(W*H), 0.0f), 255.0f);
      out[4*i+0]=out[4*i+1]=out[4*i+2]=v;
      out[4*i+3]=255;
    }
    // re‑encode
    std::vector<unsigned char> png_out;
    lodepng::encode(png_out, out, W, H);
    // reply
    dprintf(client,
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: image/png\r\n"
      "Content-Length: %zu\r\n\r\n",
      png_out.size());
    write(client, png_out.data(), png_out.size());
    close(client);
}

int main() {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt=1; setsockopt(srv,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    sockaddr_in addr = {};
    addr.sin_family=AF_INET; addr.sin_port=htons(PORT);
    addr.sin_addr.s_addr=INADDR_ANY;
    bind(srv,(sockaddr*)&addr,sizeof(addr));
    listen(srv,4);

    while (1) {
        int cli = accept(srv,0,0);
        std::thread(handle_client, cli).detach();
    }
    return 0;
}

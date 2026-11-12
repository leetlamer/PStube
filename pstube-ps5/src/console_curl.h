/*
 * ps5-native-app-boilerplate - libcurl support for PS5 native titles.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PacBrew's libcurl (with OpenSSL, zlib, zstd and libpsl) links into a native
 * title, but needs console_curl.c beside it to run:
 *
 *   - getaddrinfo and friends on the system resolver (the SDK's stubs import
 *     them from a module a native title does not load: a call jumps to 0);
 *   - small stand-ins for libc functions the archives ask for;
 *   - an fcntl wrapper that answers curl's socket calls, which the console's
 *     libc refuses in a sandboxed title. Link with --wrap=fcntl:
 *     APP_WRAP_SYMBOLS += fcntl in the Makefile;
 *   - the path of the console's certificate list, for CURLOPT_CAINFO.
 *
 * In the Makefile: PACBREW_PACKAGES += libcurl and APP_WRAP_SYMBOLS += fcntl.
 * Call console_curl_setup() on every handle. See docs/UPDATE_CHECK.md.
 */

#ifndef CONSOLE_CURL_H
#define CONSOLE_CURL_H

#include <curl/curl.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* What every curl handle needs on the console; call it after curl_easy_init():
     *   CURLOPT_NOSIGNAL          no signals on the console;
     *   CURLOPT_CAINFO            console_curl_ca_file();
     *   CURLOPT_SOCKOPTFUNCTION   makes each socket non-blocking with the console's SO_NBIO
     *                             option. Without it a socket can stay blocking, and a finished
     *                             request then waits until the server closes the connection.
     * An app that needs its own CURLOPT_SOCKOPTFUNCTION calls console_curl_nonblocking() in it. */
    void console_curl_setup(CURL *easy);

    /* Sets SO_NBIO on a socket. 0 on success. */
    int console_curl_nonblocking(int socket);

    /* The console's certificate authorities as a PEM file OpenSSL reads, for
     * CURLOPT_CAINFO: /system/common/cert/CA_LIST.cer with filesystem access
     * (elevated), /<sandbox word>/common/cert/CA_LIST.cer in the sandbox.
     * Checked again on every call, so it follows a change of root. */
    const char *console_curl_ca_file(void);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_CURL_H */

#ifndef PSTUBE_NETWORK_SETTINGS_H
#define PSTUBE_NETWORK_SETTINGS_H
/* Application-only network routing. No system DNS/proxy configuration is changed. */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct console_network_settings {
    int dns_mode; /* 0 system IPv4, 1 Google DoH, 2 Cloudflare DoH, 3 custom */
    int proxy_mode; /* 0 off, 1 HTTP CONNECT, 2 HTTPS CONNECT, 3 SOCKS5 remote DNS */
    int proxy_port;
    char doh_url[513],proxy_host[254],proxy_user[129],proxy_password[129];
} console_network_settings;
void console_curl_set_network(const console_network_settings* settings);
unsigned console_curl_network_generation(void);
CURLcode console_curl_apply_network(CURL* easy,const console_network_settings* override_settings);
#ifdef __cplusplus
}
#endif

#endif /* PSTUBE_NETWORK_SETTINGS_H */

#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Expose a clean, standard interface for Moksha to call
#ifdef __cplusplus
extern "C" {
#endif

int32_t fetch_url(const char *url) {
  CURL *curl;
  CURLcode res = CURLE_FAILED_INIT;

  // Initialize the curl session
  curl = curl_easy_init();
  if (curl) {
    // Set the target URL
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // Perform the network request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }

    // Clean up the memory allocated by libcurl
    curl_easy_cleanup(curl);
  }

  // Return the status code (0 means CURLE_OK)
  return (int32_t)res;
}

void open_url_in_browser(const char* url) {
#ifdef _WIN32
    char command[512];
    snprintf(command, sizeof(command), "start %s", url);
    system(command);
#elif __APPLE__
    char command[512];
    snprintf(command, sizeof(command), "open %s", url);
    system(command);
#else // Linux
    char command[512];
    snprintf(command, sizeof(command), "xdg-open %s", url);
    system(command);
#endif
}

#ifdef __cplusplus
}
#endif

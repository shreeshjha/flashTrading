#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <vector>

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void postOrder(const std::string &symbol, int id, double price, int quantity, char side) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return;
    }
    
    // Construct JSON body
    std::string jsonBody = "{\"symbol\":\"" + symbol + "\","
                          "\"id\":" + std::to_string(id) + ","
                          "\"price\":" + std::to_string(price) + ","
                          "\"quantity\":" + std::to_string(quantity) + ","
                          "\"side\":\"" + side + "\"}";

    std::string readBuffer;
    struct curl_slist *headers = NULL;
    
    // Set proper headers
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    
    // Debug output
    std::cout << "Sending request to http://127.0.0.1:18080/add_order" << std::endl;
    std::cout << "Headers:" << std::endl;
    std::cout << "Content-Type: application/json" << std::endl;
    std::cout << "Accept: application/json" << std::endl;
    std::cout << "Body: " << jsonBody << std::endl;
    
    // Set all CURL options
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:18080/add_order");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonBody.length());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable verbose output for debugging

    // Perform request
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " 
                  << curl_easy_strerror(res) << std::endl;
    }
    else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        std::cout << "Order request complete:" << std::endl;
        std::cout << "HTTP Status: " << http_code << std::endl;
        std::cout << "Response: " << readBuffer << std::endl;
        
        if (http_code == 200) {
            std::cout << "Successfully posted order with ID " << id << std::endl;
        } else {
            std::cerr << "Server returned error code " << http_code << std::endl;
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

int main() {
    // Initialize CURL
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return 1;
    }
    
    // Wait for simulator to start
    std::cout << "Waiting for simulator to start..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Seed random number generator
    srand(time(nullptr));
    
    const std::vector<std::string> symbols = {"AAPL", "MSFT"};
    
    std::cout << "Starting to send orders..." << std::endl;
    
    while(true) {
        try {
            int orderId = rand() % 100000 + 50000;
            double price = 100.0 + (rand() % 50);
            int quantity = (rand() % 10) + 1;
            char side = (rand() % 2) == 0 ? 'B' : 'S';
            
            // Randomly select a symbol
            const std::string& symbol = symbols[rand() % symbols.size()];
            
            postOrder(symbol, orderId, price, quantity, side);
            
            // Wait between orders
            std::this_thread::sleep_for(std::chrono::seconds(3));
        } catch (const std::exception& e) {
            std::cerr << "Error in main loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    curl_global_cleanup();
    return 0;
}

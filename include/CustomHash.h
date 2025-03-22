#pragma once

#include <esp32-hal-log.h>
#include <mbedtls/md.h>

class CustomHash {
public:
    static String sha256(const String& input) {
        byte shaResult[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
        
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, (const unsigned char*)input.c_str(), input.length());
        mbedtls_md_finish(&ctx, shaResult);
        mbedtls_md_free(&ctx);
        
        String hashStr = "";
        for(int i= 0; i< 32; i++) {
            char str[3];
            sprintf(str, "%02x", (int)shaResult[i]);
            hashStr += str;
        }
        
        return hashStr;
    }
};
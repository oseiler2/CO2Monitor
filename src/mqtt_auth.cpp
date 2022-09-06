#include <mqtt_auth.h>
#include <mqtt.h>
#include <configManager.h>
#include <wifiManager.h>
#include <LittleFS.h>

#include <mbedtls/platform.h>
#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <esp_task_wdt.h>

#include <stdio.h>
#include <string.h>

// Local logging tag
static const char TAG[] = __FILE__;

namespace mqtt {

    #define KEY_SIZE 2048
    #define EXPONENT 65537

    // Clean-up after key initialization and report status
    bool finishInitKey(mbedtls_pk_context *key, mbedtls_x509write_csr *req,
                       mbedtls_entropy_context *entropy,
                       mbedtls_ctr_drbg_context *ctr_drbg, unsigned char *output_buf,
                       const char *step, int ret) {

        // Reset watchdog timer to default
        esp_task_wdt_init(CONFIG_ESP_TASK_WDT_TIMEOUT_S, true);

        mbedtls_pk_free(key);
        mbedtls_x509write_csr_free(req);
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_entropy_free(entropy);
        delete key;
        delete req;
        delete ctr_drbg;
        delete entropy;
        if (output_buf != NULL) {
            free(output_buf);
        }

        if (ret != 0) {
            char *status = (char *)malloc(1024);
            char *errMsg = (char *)malloc(1024);
            if (status != NULL && errMsg != NULL) {
                mbedtls_strerror( ret, errMsg, sizeof(errMsg));
                snprintf(status, 1024, "Failed to initialize key at %s: %s", step, errMsg);
                free(errMsg);
            } else {
                ESP_LOGE(TAG, "(status not sent due to alloc failure) Failed to initialize key at %s", step);
            }
            // waiting for https://github.com/oseiler2/CO2Monitor/pull/14/files
            //sendStatus(status)
            ESP_LOGD(TAG, "initKey finished: %s", status);
            free(status);
            return false;
        }

        //sendStatus("Key initialized successfully");
        ESP_LOGD(TAG, "Key initialized successfully");
        return true;
    }

    // Helper to write a file to disk
    bool writeFile(const char *name, unsigned char *contents) {
        File f;
        if (!(f = LittleFS.open(name, FILE_WRITE))) {
            return false;
        }

        int len = strlen((char *)contents);
        if (f.write(contents, len) != len) {
            f.close();
            return false;
        }

        f.close();
        return true;
    }

    // Attempts to initialize an RSA key (and associated CSR) for the device
    bool initKey(void) {
        ESP_LOGD(TAG, "Generating RSA private key... ");

        // Getting the entropy for the key can take a few seconds, which doesn't play well with the default (5s) watchdog timer
        // So bump it up for a bit while we generate the key - will be reset to the default value in finishInitKey above.
        esp_task_wdt_init(15, true);

        mbedtls_pk_context *key = new mbedtls_pk_context;
        mbedtls_x509write_csr *req = new mbedtls_x509write_csr;
        mbedtls_entropy_context *entropy = new mbedtls_entropy_context;
        mbedtls_ctr_drbg_context *ctr_drbg = new mbedtls_ctr_drbg_context;
        String mac = WifiManager::getMac();
        int ret = 1;

        mbedtls_ctr_drbg_init(ctr_drbg);
        mbedtls_entropy_init(entropy);
        mbedtls_pk_init(key);
        mbedtls_x509write_csr_init(req);
        mbedtls_x509write_csr_set_md_alg(req, MBEDTLS_MD_SHA256);

        ESP_LOGD(TAG, "Initializing PRNG... ");
        if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                (const unsigned char *)mac.c_str(), mac.length())) != 0) {
            return finishInitKey(key, req, entropy, ctr_drbg, NULL, "seed", ret);
        }

        ESP_LOGD(TAG, "PK setup... ");
        if ((ret = mbedtls_pk_setup(key, mbedtls_pk_info_from_type((mbedtls_pk_type_t)MBEDTLS_PK_RSA))) != 0) {
            return finishInitKey(key, req, entropy, ctr_drbg, NULL, "setup", ret);
        }

        ESP_LOGD(TAG, "Actually generating... ");
        if ((ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*key), mbedtls_ctr_drbg_random, ctr_drbg, KEY_SIZE, EXPONENT)) != 0) {
            return finishInitKey(key, req, entropy, ctr_drbg, NULL, "gen key", ret);
        }

        unsigned char *output_buf = (unsigned char *)malloc(PEM_BUFLEN);
        if (output_buf == NULL) {
            return finishInitKey(key, req, entropy, ctr_drbg, NULL, "allocate buffer", MBEDTLS_ERR_PK_ALLOC_FAILED);
        }

        memset(output_buf, 0, PEM_BUFLEN);
        ESP_LOGD(TAG, "Writing key PEM.. ");
        if( ( ret = mbedtls_pk_write_key_pem( key, output_buf, PEM_BUFLEN ) ) != 0 ) {
            return finishInitKey(key, req, entropy, ctr_drbg, output_buf, "serialize key", ret);
        }
        if (!writeFile(MQTT_CLIENT_NEW_KEY_FILENAME, output_buf)) {
            return finishInitKey(key, req, entropy, ctr_drbg, output_buf, "write key", MBEDTLS_ERR_PK_FILE_IO_ERROR);
        }

        ESP_LOGD(TAG, "Generating CSR... ");
        char cn[50];
        snprintf(&cn[0], 50, "CN=%s,O=CO2Monitor", mac);
        if ((ret = mbedtls_x509write_csr_set_subject_name( req, cn)) != 0 ) {
            return finishInitKey(key, req, entropy, ctr_drbg, output_buf, "set subject name", ret);
        }
        mbedtls_x509write_csr_set_key(req, key);

        ESP_LOGD(TAG, "Writing CSR PEM... ");
        memset( output_buf, 0, PEM_BUFLEN );
        if ((ret = mbedtls_x509write_csr_pem(req, output_buf, PEM_BUFLEN, mbedtls_ctr_drbg_random, ctr_drbg)) < 0) {
            return finishInitKey(key, req, entropy, ctr_drbg, output_buf, "serialize csr", ret);
        }
        if (!writeFile(MQTT_CLIENT_CSR_FILENAME, output_buf)) {
            return finishInitKey(key, req, entropy, ctr_drbg, output_buf, "write csr", MBEDTLS_ERR_PK_FILE_IO_ERROR);
        }

        ESP_LOGD(TAG, "Key generated.");
        return finishInitKey(key, req, entropy, ctr_drbg, output_buf, "", 0);
    }

}
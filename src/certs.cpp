#include <mqtt_auth.h>
#include <mqtt.h>
#include <configManager.h>
#include <wifiManager.h>
#include <LittleFS.h>

#include <mbedtls/platform.h>
#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
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

    // Trims any trailing space/newlines and ensures null terminated.
    // Buf must be at least buflen+1 so there's space to add a null if required.
    int trimAndNull(unsigned char *buf, int buflen) {
        while (buf[buflen-1] == '\n' || buf[buflen-1] == ' ') {
            buf[buflen-1] = '\0';
            buflen--;
        }
        if (buf[buflen-1] != '\0') {
            buf[buflen] = '\0';
            buflen++;
        }
        return buflen;
    }

    // Helper to read a PEM file from disk.
    int readPEM(const char *filename, unsigned char *buf, int buflen) {
        File f;
        if (!(f = LittleFS.open(filename, FILE_READ))) {
            ESP_LOGE(TAG, "could not read %s", filename);
            return -1;
        }

        if (buflen < f.size()+1) {
            ESP_LOGE(TAG, "Insufficient buffer to read %s", filename);
            return -1;
        }
        int read = f.read((uint8_t *)buf, buflen);
        if (read != f.size()) {
            ESP_LOGE(TAG, "PEM read from %s failed", filename);
            return -1;
        }
        f.close();

        // Strip any trailing newline, ensure null terminated.
        return trimAndNull(buf, read);
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

    // Clean-up after certificate installation and report status
    bool finishInstallCert(mbedtls_x509_crt *cacert, mbedtls_x509_crt *crt, mbedtls_x509_crl *cacrl,
                           mbedtls_pk_context *pk, mbedtls_ctr_drbg_context *ctr_drbg, const char *step, int ret) {

        mbedtls_x509_crt_free(cacert);
        mbedtls_x509_crt_free(crt);
        mbedtls_pk_free(pk);
        mbedtls_ctr_drbg_free(ctr_drbg);
        delete cacert;
        delete crt;
        delete pk;
        delete ctr_drbg;

        if (ret != 0) {
            char *status = (char *)malloc(1024);
            char *errMsg = (char *)malloc(1024);
            if (status != NULL && errMsg != NULL) {
                mbedtls_strerror( ret, errMsg, 1024);
                snprintf(status, 1024, "Failed to install certificate at %s: %s", step, errMsg);
                free(errMsg);
            } else {
                ESP_LOGE(TAG, "(status not sent due to alloc failure) Failed to install certificate at %s", step);
            }
            // waiting for https://github.com/oseiler2/CO2Monitor/pull/14/files
            //sendStatus(status)
            ESP_LOGD(TAG, "installCert finished: %s", status);
            free(status);
            return false;
        }

        //sendStatus("Key initialized successfully");
        ESP_LOGD(TAG, "Installed new certificate with Subject: %s", step);
        return true;
    }

    // Attempts to install the provided cert
    bool installCert(char *cert_pem, const unsigned int len) {
        ESP_LOGD(TAG, "Installing certificate... ");

        mbedtls_ctr_drbg_context *ctr_drbg = new mbedtls_ctr_drbg_context;
        mbedtls_x509_crt *cacert = new mbedtls_x509_crt;
        mbedtls_x509_crt *crt = new mbedtls_x509_crt;
        mbedtls_x509_crl *cacrl = new mbedtls_x509_crl;
        mbedtls_pk_context *pk = new mbedtls_pk_context;

        int ret = 1;

        mbedtls_ctr_drbg_init(ctr_drbg);
        mbedtls_x509_crt_init(cacert);
        mbedtls_x509_crt_init(crt);
        mbedtls_pk_init(pk);
        memset(cacrl, 0, sizeof(mbedtls_x509_crl));

        // Load CA
        unsigned char *pem_buf = (unsigned char *)malloc(PEM_BUFLEN);
        if (pem_buf == NULL) {
            finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "ca buffer",  MBEDTLS_ERR_X509_ALLOC_FAILED);
        }
        int ca_len = readPEM(MQTT_ROOT_CA_FILENAME, pem_buf, PEM_BUFLEN);
        if (ca_len > 0) {
            ret = mbedtls_x509_crt_parse(cacert, pem_buf, ca_len);
        } else {
            ret = MBEDTLS_ERR_X509_FILE_IO_ERROR;
        }
        free(pem_buf);
        if (ret < 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "loadca", ret);
        }

        // Load cert
        int pem_len = trimAndNull((unsigned char *)cert_pem, len);
        if ((ret = mbedtls_x509_crt_parse(crt, (const unsigned char *)cert_pem, pem_len)) < 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "parse cert", ret);
        }
        if (ret != 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "bad cert", 0);
        }

        // Validate against CA.
        uint32_t flags;
        if ((ret = mbedtls_x509_crt_verify(crt, cacert, cacrl, NULL, &flags, NULL, NULL)) != 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "verify ca", ret);
        }

        // Validate against key
        pem_buf = (unsigned char *)malloc(PEM_BUFLEN);
        if (pem_buf == NULL) {
            finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "key buffer",  MBEDTLS_ERR_X509_ALLOC_FAILED);
        }
        int key_len = readPEM(MQTT_CLIENT_NEW_KEY_FILENAME, pem_buf, PEM_BUFLEN);
        if (key_len > 0) {
            ret = mbedtls_pk_parse_key(pk, pem_buf, key_len, NULL, 0);
        } else {
            ret = MBEDTLS_ERR_X509_FILE_IO_ERROR;
        }
        free(pem_buf);
        if (ret != 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "load key", ret);
        }
        if ((ret = mbedtls_pk_check_pair(&crt->pk, pk)) != 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "verify key", ret);
        }

        // Backup existing key/cert before installing
        if (LittleFS.exists(MQTT_CLIENT_KEY_FILENAME)) {
            if (!LittleFS.rename(MQTT_CLIENT_KEY_FILENAME, MQTT_CLIENT_OLD_KEY_FILENAME)) {
                return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "backup key", MBEDTLS_ERR_X509_FILE_IO_ERROR);
            }
        }
         if (LittleFS.exists(MQTT_CLIENT_CERT_FILENAME)) {
            if (!LittleFS.rename(MQTT_CLIENT_CERT_FILENAME, MQTT_CLIENT_OLD_CERT_FILENAME)) {
                return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "backup cert", MBEDTLS_ERR_X509_FILE_IO_ERROR);
            }
        }

        // Install new
        if (!LittleFS.rename(MQTT_CLIENT_NEW_KEY_FILENAME, MQTT_CLIENT_KEY_FILENAME)) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "write key", MBEDTLS_ERR_X509_FILE_IO_ERROR);
        }
        if (!writeFile(MQTT_CLIENT_CERT_FILENAME, (unsigned char *)cert_pem)) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "write cert", MBEDTLS_ERR_X509_FILE_IO_ERROR);
        }

        // Extract subject
        char subject_name[256];
        if ((ret = mbedtls_x509_dn_gets(&subject_name[0], sizeof(subject_name), &crt->subject)) < 0) {
            return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, "get subject", ret);
        }
        return finishInstallCert(cacert, crt, cacrl, pk, ctr_drbg, subject_name, 0);
    }

}
#ifndef _CERTS_H
#define _CERTS_H

// Haven't seen one bigger than ~1700bytes
#define PEM_BUFLEN 2048

namespace certs {
  bool initKey();
  bool installCert(char* cert_pem, const unsigned int len);
}

#endif
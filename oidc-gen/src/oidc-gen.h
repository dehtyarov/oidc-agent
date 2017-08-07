#ifndef OIDC_GEN_H
#define OIDC_GEN_H

struct oidc_provider* genNewProvider();
void freeProvider(struct oidc_provider* p);
char* getTokenEndpoint(const char* configuration_endpoint);
char* encryptProvider(const char* json, const char* password) ;
void saveExit(int exitno);

#endif // OIDC_GEN_H

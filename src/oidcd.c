#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>

#include "oidc.h"
#include "file_io.h"
#include "ipc.h"
#include "provider.h"
#include "oidc_string.h"

void sig_handler(int signo) {
  switch(signo) {
    case SIGSEGV:
      syslog(LOG_AUTHPRIV|LOG_EMERG, "Caught Signal SIGSEGV");
      break;
    default: 
      syslog(LOG_AUTHPRIV|LOG_EMERG, "Caught Signal %d", signo);
  }
  exit(signo);
}

void daemonize() {
  pid_t pid;
  if ((pid = fork ()) != 0)
    exit (EXIT_FAILURE);
  if (setsid() < 0) {
    exit (EXIT_FAILURE);
  }
  signal(SIGHUP, SIG_IGN);
  if ((pid = fork ()) != 0)
    exit (EXIT_FAILURE);
  chdir ("/");
  umask (0);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  open("/dev/null", O_RDONLY);
  open("/dev/null", O_RDWR);
  open("/dev/null", O_RDWR);
}

int main(/* int argc, char** argv */) {
  openlog("oidc-service", LOG_CONS|LOG_PID, LOG_AUTHPRIV);
  setlogmask(LOG_UPTO(LOG_DEBUG));
  // setlogmask(LOG_UPTO(LOG_NOTICE));
  signal(SIGSEGV, sig_handler);

  char* oidc_dir = getOidcDir();
  if (NULL==oidc_dir) {
    printf("Could not find an oidc directory. Please make one. I might do it myself in a future version.\n");
    exit(EXIT_FAILURE);
  }
  free(oidc_dir);

  struct connection* con = calloc(sizeof(struct connection), 1);
  ipc_init(con, "gen", "OIDC_GEN_SOCK", 1);
  struct oidc_provider* loaded_p = NULL;
  size_t loaded_p_count = 0;
  daemonize();

    ipc_bindAndListen(con);


  while(1) {
    ipc_accept_async(con, -1);
    char* provider_json = ipc_read(*(con->msgsock));
    struct oidc_provider* provider = getProviderFromJSON(provider_json);
    free(provider_json);
    if(provider!=NULL) {
      if(getAccessToken(provider, FORCE_NEW_TOKEN)!=0) {
        ipc_write(*(con->msgsock), RESPONSE_ERROR, "misconfiguration or network issues"); 
        free(provider);
        continue;
      } 
      if(isValid(provider_getRefreshToken(*provider))) {
        ipc_write(*(con->msgsock), RESPONSE_STATUS_REFRESH, "success", provider_getRefreshToken(*provider));
      } else {
        ipc_write(*(con->msgsock), RESPONSE_STATUS, "success");
      }
      loaded_p = addProvider(loaded_p, &loaded_p_count, *provider);
      free(provider);
    } else {
      ipc_write(*(con->msgsock), RESPONSE_ERROR, "json malformed");
    }
  }

  // daemonize();
  // readSavedConfig();
  // readConfig();
  // struct connection* con = initTokenSocket(); // can't call it directly after daemonizing, because the child might not have exited yet and therefore the ppid could be  wrong
  // do {
  //   unsigned int provider;
  //   for(provider=0; provider<conf_getProviderCount(); provider++) {
  //     if(getAccessToken(provider)!=0) {
  //       return EXIT_FAILURE;
  //     }
  //     logConfig();
  //     time_t expires_at = conf_getTokenExpiresAt(provider);
  //     syslog(LOG_AUTHPRIV|LOG_DEBUG, "token expires at: %ld\n",expires_at);
  //     saveConfig();
  //     if(NULL!=conf_getWattsonUrl())
  //       test(provider);
  //   }
  //   sort_provider(); // sorts provider by the time until token has to be refreshed
  //   while(tokenIsValidForSeconds(0,conf_getMinValidPeriod(0)))  {
  //     // sleep(conf_getTokenExpiresAt(0)-time(NULL)-conf_getMinValidPeriod(0));
  //     if (tryAccept(con, tokenIsValidForSeconds(0,conf_getMinValidPeriod(0)))>0) {
  //       //accepted a connection so we can communicate
  //       communicate(con); 
  //     }
  //   }
  // } while(1);
  return EXIT_FAILURE;
}




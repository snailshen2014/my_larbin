// Larbin
// Sebastien Ailleret
// 15-11-99 -> 04-12-01

#include <iostream>
#include <errno.h>
#include <sys/types.h>

#include <adns.h>

#include "options.h"

#include "global.h"
#include "utils/Fifo.h"
#include "utils/debug.h"
#include "fetch/site.h"
using namespace std;
/* Opens sockets
 * Never block (only opens sockets on already known sites)
 * work inside the main thread
 */
void fetchOpen () {
  static time_t next_call = 0;
  if (global::now < next_call) { // too early to come back
    return;
  }
  int cont = 1;
  while (cont && global::freeConns->isNonEmpty()) {
    IPSite *s = global::okSites->tryGet();
    if (s == NULL) {cout<<"[fetchOpen()]global::okSites->tryGet() null"<<endl;
      cont = 0;
    } else {cout<<"[fetchOpen()]global::okSites->tryGet() not null,fetch()"<<endl;
      next_call = s->fetch();
      cont = (next_call == 0);
    }
  }
}

/* Opens sockets
 * this function perform dns calls, using adns
 */
void fetchDns () {
  // Submit queries
  //why global::freeConns->isNonEmpty() is not empty? shenyj
  while (global::nbDnsCalls<global::dnsConn
         && global::freeConns->isNonEmpty()
         && global::IPUrl < maxIPUrls) { // try to avoid too many dns calls
    NamedSite *site = global::dnsSites->tryGet();
    if (site == NULL) {
      break;
    } else {cout<<"[fetchDns()]global::dnsSites->tryGet() not  null,newquery()"<<endl;
      site->newQuery();
    }
  }

  // Read available answers
  while (global::nbDnsCalls && global::freeConns->isNonEmpty()) {
    NamedSite *site;
    adns_query quer = NULL;
    adns_answer *ans;
    int res = adns_check(global::ads, &quer, &ans, (void**)&site);
    if (res == ESRCH || res == EAGAIN) {
      // No more query or no more answers
      cout<<"[fetchDns ()] No more query or no more answers"<<endl;
      break;
    }
    global::nbDnsCalls--;
    site->dnsAns(ans);
    free(ans); // ans has been allocated with malloc
  }
}

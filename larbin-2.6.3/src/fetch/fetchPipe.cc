// Larbin
// Sebastien Ailleret
// 15-11-99 -> 19-11-01

#include <iostream>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "options.h"

#include "types.h"
#include "global.h"
#include "utils/url.h"
#include "utils/text.h"
#include "utils/string.h"
#include "utils/connexion.h"
#include "fetch/site.h"
#include "fetch/file.h"
#include "interf/output.h"

#include "utils/debug.h"
using namespace std;
static void pipeRead (Connexion *conn);
static void pipeWrite (Connexion *conn);
static void endOfFile (Connexion *conn);


/** Check timeout
 */
void checkTimeout () {
  for (uint i=0; i<global::nb_conn; i++) {
	Connexion *conn = global::connexions+i;
    if (conn->state != emptyC) {
      if (conn->timeout > 0) {
        if (conn->timeout > timeoutPage) {
          conn->timeout = timeoutPage;
        } else {
          conn->timeout--;
        }
      } else {
        // This server doesn't answer (time out)
        conn->err = timeout;
        endOfFile(conn);
      }
    }
  }
}

/** read all data available
 * fill fd_set for next select
 * give back max fds
 */
void checkAll () {
  // read and write what can be
  //The connexions is equal global::freeConns by shenyj
  for (uint i=0; i<global::nb_conn; i++) {
    Connexion *conn = global::connexions+i;
    switch (conn->state) {
    case connectingC:
    case writeC:
      if (global::ansPoll[conn->socket]) {
        // trying to finish the connection
        cout<<"[checkALL],i="<<i<<"connectingC,or writeC,pipeWrite() begin"<<endl;
        pipeWrite(conn);
		cout<<"[checkALL],i="<<i<<"connectingC,or writeC,pipeWrite() end"<<endl;
      }
      break;
    case openC:
      if (global::ansPoll[conn->socket]) {
        // The socket is open, let's try to read it
        cout<<"[checkALL],i="<<i<<"openC,pipeRead() begin"<<endl;
        pipeRead(conn);
		cout<<"[checkALL],i="<<i<<"openC,pipeRead() end"<<endl;
      }
      break;
    }
  }

  // update fd_set for the next select
  for (uint i=0; i<global::nb_conn; i++) {
    int n = (global::connexions+i)->socket;
    switch ((global::connexions+i)->state) {
    case connectingC:
    case writeC:
      global::setPoll(n, POLLOUT);
      break;
    case openC:
      global::setPoll(n, POLLIN);
      break;
    }
  }
}

/** The socket is finally open !
 * Make sure it's all right, and write the request
 */
static void pipeWrite (Connexion *conn) {
  cout<<"pipeWrite:,string="<<conn->request.getString()<<endl;
  int res = 0;
  int wrtn, len;
  socklen_t size = sizeof(int);
  cout<<"[pipeWrite]:state="<<(int)conn->state<<endl;
  switch (conn->state) {
  case connectingC:
	// not connected yet
	getsockopt(conn->socket, SOL_SOCKET, SO_ERROR, &res, &size);
	if (res) {
	  // Unable to connect
      conn->err = noConnection;
	  endOfFile(conn);
	  return;
	}
	// Connection succesfull
	conn->state = writeC;
	// no break
  case writeC:
	// writing the first string
	len = strlen(conn->request.getString());
	wrtn = write(conn->socket, conn->request.getString()+conn->pos,
                 len - conn->pos);
	if (wrtn >= 0) {
      addWrite(wrtn);
#ifdef MAXBANDWIDTH
      global::remainBand -= wrtn;
#endif // MAXBANDWIDTH
	  conn->pos += wrtn;
	  if (conn->pos < len) {
		// Some chars of this string are not written yet
		return;
	  }cout<<"write ok"<<endl;
	} else {cout<<"wirte error"<<endl;
	  if (errno == EAGAIN || errno == EINTR || errno == ENOTCONN) {
		// little error, come back soon
		return;
	  } else {
		// unrecoverable error, forget it
        conn->err = earlyStop;
		endOfFile(conn);
		return;
	  }
	}
	// All the request has been written
	conn->state = openC;
  }
}

/** Is there something to read on this socket
 * (which is open)
 */
static void pipeRead (Connexion *conn) {

  int p = conn->parser->pos;
  int size = read (conn->socket, conn->buffer+p, maxPageSize-p-1);
  #ifdef SHENYJ
  cout<<"pipeRead:,read buf="<<conn->buffer+p<<",size="<<size<<endl;
  #endif
  
  switch (size) {
  case 0:
    // End of file,parse the html
    if (conn->parser->endInput())
      conn->err = (FetchError) errno;cout<<"pipeRead endOfFile()"<<endl;
	//save to disk,if url is html .added by shenyj
	endOfFile(conn);
    break;
  case -1:
    switch (errno) {
    case EAGAIN:
      // Nothing to read now, we'll try again later
      break;
    default:
      // Error : let's forget this page
      cout<<"[pipeRead]:Error : let's forget this page"<<endl;
      conn->err = earlyStop;
      endOfFile(conn);
      break;
    }
    break;
  default:
    // Something has been read
    conn->timeout += size / timeoutIncr;
    addRead(size);
#ifdef MAXBANDWIDTH
    global::remainBand -= size;
#endif // MAXBANDWIDTH
    if (conn->parser->inputHeaders(size) == 0) {
      // nothing special
      if (conn->parser->pos >= maxPageSize-1) {
        // We've read enough...
        conn->err = tooBig;cout<<"[pipeRead]:toobig"<<endl;
        endOfFile(conn);
      }
    } else {//err40X
      // The parser does not want any more input (errno explains why)
      cout<<"[pipeRead]:The parser does not want any more input (errno explains why)"<<endl;
      conn->err = (FetchError) errno;cout<<"error no="<<int(errno)<<endl;
      endOfFile(conn);
    }
    break;
  }
}


/* What are we doing when it's over with one file ? */

#ifdef THREAD_OUTPUT
#define manageHtml() global::userConns->put(conn)
#else // THREAD_OUTPUT
#define manageHtml() \
    endOfLoad((html *)conn->parser, conn->err); \
    conn->recycle(); \
    global::freeConns->put(conn)
#endif // THREAD_OUTPUT

static void endOfFile (Connexion *conn) {
  crash("End of file");
  conn->state = emptyC;
  close(conn->socket);
  if (conn->parser->isRobots) {cout<<"robots"<<endl;
	// That was a robots.txt
    robots *r = ((robots *) conn->parser);
	r->parse(conn->err != success);
	r->server->robotsResult(conn->err);
	conn->recycle();
	global::freeConns->put(conn);
  } else {cout<<" that was an html page"<<endl;
    // that was an html page
    manageHtml();
  }
}

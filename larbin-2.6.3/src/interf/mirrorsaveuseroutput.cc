// Larbin
// Sebastien Ailleret
// 07-12-01 -> 10-12-01

#include <iostream>
#include <string.h>
#include <unistd.h>

//parse html for txt file
#include <htmlcxx/html/ParserDom.h>
#include <fstream>

#include "options.h"

#include "types.h"
#include "global.h"
#include "fetch/file.h"
#include "utils/text.h"
#include "utils/debug.h"
#include "interf/output.h"
using namespace std;
using namespace htmlcxx;

static char *fileName;
static uint endFileName;

/** A page has been loaded successfully
 * @param page the page that has been fetched
 */
void loaded (html *page) {printf("Mirrorsaveuseroutput loaded\n");
  // get file name and create needed directories
  url *u = page->getUrl();
  uint p = u->getPort();
  char *h = u->getHost();
  char *f = u->getFile();
  cout<<"[mirror]load running...fileName="<<fileName<<",url="<<p<<","<<h<<","<<f<<endl;
  // update dir name
  uint d = u->hostHashCode() % nbDir;
  for (int i=2; i<7; i++) {
    fileName[endFileName-i] = d % 10 + '0';
    d /= 10;
  }
  // set file name
  uint len = endFileName;
  if (p == 80)
    len += sprintf(fileName+endFileName, "%s%s", h, f);
  else
    len += sprintf(fileName+endFileName, "%s:%u%s", h, p, f);
  cout<<"[mirror] fileName="<<fileName<<endl;
  // make sure the path of the file exists
  bool cont = true;
  struct stat st;
  while (cont) {
    len--;
    while (fileName[len] != '/') len--;
    fileName[len] = 0;
    cont = stat(fileName, &st); // this becomes true at least for saveDir
    fileName[len] = '/';
  }
  cont = true;
  while (cont) {
    len++;
    while (fileName[len] != '/' && fileName[len] != 0) len++;
    if (fileName[len] == '/') {
      fileName[len] = 0;
      if (mkdir(fileName, S_IRWXU) != 0) perror("trouble while creating dir");
      fileName[len] = '/';
    } else { // fileName[len] == 0
      cont = false;
    }
  }
  if (fileName[len-1] == '/') {
    strcpy(fileName+len, indexFile);
  }
  // open fds and write file
  int fd = creat(fileName, S_IRWXU);cout<<"create filename="<<fileName<<endl;
  if (fd >= 0) {
    // some url mysteries might prevent this from being possible
    // ex: if http://bbs.computoredge.com/ceo
    // and http://bbs.computoredge.com/ceo/ both exist
    ecrireBuff(fd, page->getPage(), page->getLength());
    close(fd);
  }
  /*
  //added by shenyj for write txt file
  string file(fileName);
  if(file.find(".html") != string::npos || file.find(".shtml") != string::npos)
 	 writeTxtFile(page->getPage());
  */
  
}

/** The fetch failed
 * @param u the URL of the doc
 * @param reason reason of the fail
 */
void failure (url *u, FetchError reason) {
}

/** initialisation function
 */
void initUserOutput () {
  mkdir(saveDir, S_IRWXU);
  endFileName = strlen(saveDir);
  fileName = new char[endFileName+maxUrlSize+50];
  strcpy(fileName, saveDir);
  if (fileName[endFileName-1] != '/') fileName[endFileName++] = '/';
  strcpy(fileName+endFileName, "d00000/");
  endFileName += 7; // indique le premier char a ecrire
}

/** stats, called in particular by the webserver
 * the webserver is in another thread, so be careful
 * However, if it only reads things, it is probably not useful
 * to use mutex, because incoherence in the webserver is not as critical
 * as efficiency
 */
void outputStats(int fds) {
  ecrire(fds, "Nothing to declare");
}


/** write h1,p of the html file to txt file
*/
void writeTxtFile(char* page){
	string htmlfile = fileName;
	string txtfile;
	if (htmlfile.find(".html") != string::npos)
  		 txtfile =htmlfile.replace(htmlfile.find(".html"),5,".txt");
	if (htmlfile.find(".shtml") != string::npos)
  		 txtfile =htmlfile.replace(htmlfile.find(".shtml"),6,".txt");
	string html = page;
	ofstream txtstream(txtfile.c_str());
  	HTML::ParserDom parser;
  	tree<HTML::Node> dom = parser.parseTree(html);
  
  	//Dump all links in the tree
  	tree<HTML::Node>::iterator it = dom.begin();
  	tree<HTML::Node>::iterator end = dom.end();
  	for (; it != end; ++it)
  	{
		//date
		/*if (!it->isTag() && !it->isComment() && strcasecmp(it.node->parent->data.tagName().c_str(), "div") == 0)
		{
			string s = it->text();
			if (s.find("-") == string::npos)
				continue;
			string date;
			if(s.size()>10){
				date = s.substr(s.size()-10,10);
				txtstream << date<<endl;
			}
			
		}*/
		if (strcmp(it->tagName().c_str(), "div") == 0)
		{
			it->parseAttributes();
			map<string, string> attris = it->attributes();
			map<string, string>::iterator iter = attris.begin();
			for (iter; iter != attris.end(); iter++)
			{
				string::size_type idx = iter->second.find("date");
				if (idx != string::npos)
				{
					tree<HTML::Node>::iterator nextIter = ++it;
					string s = nextIter->text();
					if (s.find("-") == string::npos){
						it--;
						continue;
					}
					string date;
					if(s.size()>10){
						date = s.substr(s.size()-10,10);
						txtstream << date<<endl;
					}
					it--;
				}
			}
		}
		
		//title
		if (!it->isTag() && !it->isComment() && strcasecmp(it.node->parent->data.tagName().c_str(),"h1") == 0)
		{

			txtstream<<it->text()<<endl;
		}
		if (!it->isTag() && !it->isComment() && strcasecmp(it.node->parent->data.tagName().c_str(),"title") == 0)
		{

			txtstream<<it->text()<<endl;
		}
		//paragraph <p>
		if (!it->isTag() && !it->isComment() && strcasecmp(it.node->parent->data.tagName().c_str(), "p") == 0)
		{
			string par = it->text();
			string::size_type pos;
			while ((pos = par.find("&nbsp;")) != string::npos)
				par = par.replace(pos,6,"");
			txtstream << par <<endl;
		}
		//paragraph <span>
		if (!it->isTag() && !it->isComment() && strcasecmp(it.node->parent->data.tagName().c_str(), "span") == 0)
		{
			string par = it->text();
			string::size_type pos;
			while ((pos = par.find("&nbsp;")) != string::npos)
				par = par.replace(pos,6,"");
			txtstream << par <<endl;
		}
  	}
	txtstream.close();
		
}

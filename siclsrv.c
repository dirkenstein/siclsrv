#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __linux__
#include <gpib/ib.h>
#else
#include "ni4882.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/select.h> 

#define SA struct sockaddr 

/* igpibbusstatus values */
#define I_GPIB_BUS_REM         1
#define I_GPIB_BUS_SRQ         2
#define I_GPIB_BUS_NDAC        3
#define I_GPIB_BUS_SYSCTLR     4
#define I_GPIB_BUS_ACTCTLR     5
#define I_GPIB_BUS_TALKER      6
#define I_GPIB_BUS_LISTENER    7
#define I_GPIB_BUS_ADDR        8
#define I_GPIB_BUS_LINES       9

#define I_TERM_MAXCNT      1
#define I_TERM_CHR         2
#define I_TERM_END         4

const char * defboardId = "GPIB0";
const int defboardnum = 0;
static  int boardnum = 0;


static int boardaddr = -1;
static int xboardaddr  = -1;
  
static int fifo = 1;

const char * boardId = NULL;

//FILE * dump = NULL;

static int deb = 0;

void hexdump(const char *pre, const void *data, int size);

int iopen_f (int nargs, char ** argl, int * arglens, int nret, char **retl, int * retlens)

{
     char * ptr;
     char * boardname = strtok_r(argl[0], ",", &ptr);
     char * caddr = strtok_r(NULL, ",", &ptr); 
     int addr = -1;
     int res = -1;
     if(caddr) addr = atoi(caddr);
     if (deb > 1) fprintf(stderr, "board %s addr %d id %s\n", boardname, addr, boardId);
     if (addr == -1) {
      res = ibfind(boardId);
      if (res != -1) {
        if (res == 0)
          xboardaddr= res+32768;
        else
          xboardaddr= res;
        boardaddr = res;
      }
      res = xboardaddr;
     } else res = ibdev(boardnum, addr, 0, 11, 1, 0);
     return res;
}

int igetintfsess_f (int nargs, char ** argl, int * arglens, int nret, char **retl,  int * retlens)
{
       int id = atoi(argl[0]);
       int res = -1;
       if (boardaddr != -1) res = xboardaddr;
       else {
         if (deb > 1) fprintf(stderr, "find board %s\n", boardId);
         res = ibfind(boardId);
       }
       if (deb > 1) fprintf(stderr, "igetintfses %d res %d\n", id, res);
       return res;
}

int ireadstb_f(int nargs, char **argl, int * arglens, int nret, char **retl,  int * retlens)
{
       int id = atoi(argl[0]);
       int res = -1;
       char p;
       if (id == xboardaddr) id = boardaddr;
       res = ibrsp(id, &p);
       if (deb > 1) fprintf(stderr, "ireadstb %d res %d stb %d\n", id, res, (int)p);
       sprintf(retl[0], "%d", (int)p);
       return res;
}

int iclear_f (int nargs, char **argl, int * arglens, int nret, char **retl, int *retlens)
{
   int id = atoi(argl[0]);
   int res = -1;
   if (id == xboardaddr)  {
      id = boardaddr;
      SendIFC(boardnum);
      res = 0;
   }
   else 
      res = ibclr(id);
   if (deb > 1) fprintf(stderr, "iclear %d res %d\n", id, res);
   if (res & ERR) res = -1;
   else res = 0;
   return res;
}

int iclose_f (int nargs, char **argl, int * arglens, int nret, char **retl,  int * retlens)
{
    int id = atoi(argl[0]);
    if (id == xboardaddr) id = boardaddr;
     if (deb > 1) fprintf (stderr, "iclose %d do nothing\n", id);
    int res = 0;
    return res;
}

int ihint_f(int nargs, char **argl, int * arglens, int nret, char **retl, int * retlens)
{
    int id = atoi(argl[0]);
    int hint = atoi(argl[1]);
    if (id == xboardaddr) id = boardaddr;
    if (deb > 1) fprintf (stderr, "ihint %d %d do nothing\n", id, hint);
    //do nothing
    return 0;
}

int itimeout_f(int nargs, char **argl, int * arglens, int nret, char **retl, int * retlens)
{
       int id = atoi(argl[0]);
       int tmo = atoi(argl[1]);
       int res = -1;
       if (id == xboardaddr) id = boardaddr;
       if (tmo == 0) tmo = 0;
       else if (tmo == 1) tmo = 5;
       else if (tmo > 1 && tmo <= 3) tmo = 6; 
       else if (tmo > 3 && tmo <= 10) tmo = 7; 
       else if (tmo > 10 && tmo <= 30) tmo = 8; 
       else if (tmo > 10 && tmo <= 100) tmo = 9; 
       else if (tmo > 100 && tmo <= 300) tmo = 10; 
       else if (tmo > 300 && tmo <= 1000) tmo = 11; 
       else if (tmo > 1000 && tmo <= 3000) tmo = 12; 
       else if (tmo > 3000 && tmo <= 10000) tmo = 13; 
       else if (tmo > 10000 && tmo <= 30000) tmo = 14; 
       else if (tmo > 30000) tmo = 15; 
       if (deb > 1) fprintf (stderr, "itimeout %d %d\n", id, tmo);
       //res = ibtmo(id, tmo);
       res = ibconfig((id), IbcTMO, (tmo));

       if (res & ERR) res = -1;
       else res = 0;
       return res;
}

int itermchr_f(int nargs, char **argl, int * arglens, int nret, char **retl, int * retlens)
{
       int id = atoi(argl[0]);
       int tchr = atoi(argl[1]);
       int res = -1;
       if (id == xboardaddr) id = boardaddr;
       if (tchr != -1) {
         //res = ibeos(id, 0x14| tchr);
         res = ibconfig((id), IbcEOSrd, 1);
         res = ibconfig((id), IbcEOSchar, tchr);
       } else {
         //res = ibeos(id, 0);
         res = ibconfig((id), IbcEOSrd, 0);
         res = ibconfig((id), IbcEOSchar, 0);
       }
       if (res & ERR) res = -1;
       else res = 0;
       return res;
}

int igpibbusstatus_f (int nargs, char **argl, int * arglens, int nret, char **retl, int * retlens)
{ 
       int id = atoi(argl[0]);
       int req = atoi(argl[1]);
       short lines;
       short srqi;
       int sta;
       int res = -1;
       if (id == xboardaddr) id = boardaddr;
       if (req == I_GPIB_BUS_SRQ) {
          TestSRQ (boardnum, &srqi);
          sta = srqi; 
          res = 0;
       } else {
          res = iblines (id, &lines);
          fprintf(stderr, "igpibbusstatus FIXME %d %d\n", id, req);
          sta = lines;
       }
       if (deb > 1) fprintf (stderr, "igpibbusstatus %d %d\n", id, req);
       if (res & ERR) res = -1;
       else res = 0;  
       int l = sprintf(retl[0], "%d", sta);
       return res;
    
} 

int igpibppoll_f (int nargs, char **argl, int * arglens, int nret, char **retl, int * retlens)
{     
       int id = atoi(argl[0]);
       int res = -1;
       short polres = 0;
       if (id == xboardaddr) id = boardaddr;
       PPoll(boardnum, &polres); 
       if (deb > 1) fprintf (stderr, "PPoll %d %d\n", id, polres);
       res = 0;
       int l = sprintf(retl[0], "%d", (int)polres);
       return res;
}

int igpibsendcmd_f(int nargs, char **argl, int *arglens, int nret, char **retl,   int * retlens) 
{
       int id = atoi(argl[0]);
       int len = atoi(argl[1]);
       int res = -1;

       if (id == xboardaddr) { 
          id = boardaddr;
	  //SendCmds(boardnum, argl[2], len);
          //res = 0;
          res = ibcmd(id, argl[2], len);
        } else
          res = ibcmd(id, argl[2], len);
       if (deb > 1) fprintf (stderr, "igpibsendcmd %d %d res %d err %d\n", id, len,res, ThreadIberr());
       if (res & ERR) res = -1;
       else res = 0;
       return res;
}

static int iseoi = 0;

int iwrite_f (int nargs, char **argl, int *arglens, int nret, char **retl, int *retlens)
{
       int res = -1;
       int id = atoi(argl[0]);
       int len = atoi(argl[1]);
       int eoi = atoi(argl[2]);
       if (id == xboardaddr) id = boardaddr;
       //if (iseoi != eoi) {
         res = ibconfig(id, IbcEOT, eoi);
         if (res & ERR) res = -1;
         else res = 0;
       //  iseoi = eoi;
       //}
       //printf("iwrite id %d ibconfig %d res %d\n", id, eoi, res);
       if (deb > 1) fprintf(stderr, "iwrite id %d len %d arglen %d eoi %d\n", id, len,arglens[3], eoi);
       //if (!dump) dump = fopen("dump.bin", "wb");
       //if (len == 4096 || len == 1791) {
       //  fwrite(argl[3], len, 1, dump);
       //  fflush(dump);
       //}
       res = ibwrt(id, argl[3], len);
    
       if (deb > 1) fprintf(stderr, "iwrite id %d len %d arglen %d res %d\n", id, len,arglens[3], res);
       if (res & ERR) res = -1;
       else res = 0;
       int l = sprintf(retl[0], "%d",ThreadIbcnt());
       return res;
}

int iread_f (int nargs, char **argl, int *arglens, int nret, char **retl, int *retlens)
{
       int id = atoi(argl[0]);
       int len = atoi(argl[1]);
       int res = -1;
       int goteoi = 0;
       if (id == xboardaddr) id = boardaddr;
       free(retl[2]);
       retl[2] = malloc(len);
       res = ibrd(id, retl[2], len); 
       if (deb > 1) fprintf(stderr, "iread id %d len %d res %d err %d\n", id, len, res, ThreadIberr());
       if (res & END) goteoi = 1;
       if (res & ERR) res = -1;
       else res = 0;
       int retln = ThreadIbcnt();
       int reason = retln == len  && !goteoi ?  I_TERM_MAXCNT : I_TERM_END;
       sprintf(retl[0], "%d", reason); 
       sprintf(retl[1], "%d", retln);
       retlens[2] = retln;
       return res;
}


typedef struct _fcall {
  const char * name;
  int (*function) (int, char**, int *, int, char**, int *);
  int nargs;
  int nrets;
} fcall_t;

fcall_t fns [] = {{"iopen", iopen_f, 1, 0 },
                  {"igetinftsess", igetintfsess_f, 1, 0},
                  {"ireadstb", ireadstb_f, 1, 1},
                  {"iclear", iclear_f, 1, 0 },
                  {"iclose", iclose_f, 1, 0 },
                  {"ihint", ihint_f, 2, 0},
                  {"itimeout", itimeout_f, 2, 0},
                  {"itermchr", itermchr_f, 2, 0},
                  {"igpibbusstatus", igpibbusstatus_f, 2, 1},
                  {"igpibppoll", igpibppoll_f, 1, 1},
                  {"igpibsendcmd", igpibsendcmd_f, 3, 0},
                  {"iwrite", iwrite_f, 4, 1}, 
                  {"iread", iread_f, 2, 3}, 
                  {NULL, NULL, 0, 0}};


typedef struct _fnargs {
	char * fname;
	int nargs;
	char ** argl;
	int * arglen;
        char * argp, *bufp,*arg;
        int prfx, instr, maxargs;
} fnargs_t;

fnargs_t * mkfnargs(int n)
{
	
    fnargs_t * fnap = malloc(sizeof (fnargs_t));
    fnap->argl = malloc (n * sizeof(char *));
    fnap->arglen = malloc (n * sizeof(int *));
    fnap->maxargs = n;
    fnap->nargs = 0;
    fnap->fname = NULL;
    fnap->argp = NULL;
    fnap->bufp = NULL;
    fnap->arg = NULL;
    fnap->prfx = fnap->instr = 0;
    return fnap;
}


int parsargs(char * buf, int sz, fnargs_t * fnap)
{
     int argmem = 10;
     if (!fnap->bufp) fnap->bufp = buf;
     if (!fnap->fname) {
       while ((*fnap->bufp == ' ' || *fnap->bufp == '\t') && *fnap->bufp &&(fnap->bufp -buf < sz)) {fnap->bufp++;}
       fnap->fname = fnap->bufp;
       fnap->bufp++;
       while (*fnap->bufp != ' ' && *fnap->bufp != '\t' && *fnap->bufp && (fnap->bufp -buf < sz)) {fnap->bufp++; }
       *fnap->bufp = 0;
       fnap->bufp++;
     }
     if (!fnap->fname) return 0; 
     if (fnap->fname && !*fnap->fname) return 0; 
     if (!*fnap->bufp || (fnap->bufp -buf > sz)) return 0; 
     if (deb > 3) fprintf(stderr, "bufp [%s]\n", fnap->bufp);
     if (!fnap->arg) fnap->arg = fnap->bufp;
     while ((fnap->argp = strpbrk(fnap->bufp, ",\"\\#\n \t")) && (fnap->argp - buf < sz)) {
       if ((*fnap->argp == ',' || *fnap->argp == ' ' || *fnap->argp == '\t' )&& fnap->instr) {
          if (deb > 3) fprintf(stderr, "a bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
        }else if (*fnap->argp == ' ' || *fnap->argp == '\t') {
          *fnap->argp = 0;
          fnap->arg = fnap->argp+1; 
        }else if (*fnap->argp == ',') {
          *fnap->argp = 0;
          if (deb > 3) fprintf(stderr, "b bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
          fnap->argl[fnap->nargs] = fnap->arg;
          fnap->bufp = fnap->argp +1;
          fnap->arg = fnap->bufp;
          fnap->arglen[fnap->nargs] = -1; /* string */
          fnap->nargs++;
       } else if (*fnap->argp == '\\' && fnap->instr) {
          if (deb > 3) fprintf(stderr, "c bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
          fnap->prfx = 1;
       } else if (*fnap->argp == '\"'  && fnap->prfx) {
          if (deb > 3) fprintf(stderr, "d bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
         fnap->prfx = 0;
         if (*(fnap->argp -1) != '\\') fnap->instr = !fnap->instr;  
         else {
           memcpy (fnap->argp -1, fnap->argp, sz - (fnap->argp - buf));
         }
       } else if (*fnap->argp == '\"') {
          if (deb > 3) fprintf(stderr, "e bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
          fnap->instr = !fnap->instr;
       } else if (*fnap->argp == '\\') {
          if (deb > 3) fprintf(stderr, "f bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
          fnap->prfx = 1;
       }else if (*fnap->argp == '#' && !fnap->instr) {
          if (deb > 3) fprintf(stderr, "g bufp [%s] argp[%s] arg [%s]\n", fnap->bufp, fnap->argp, fnap->arg);
          *fnap->argp = 0;
          fnap->arglen[fnap->nargs] = atoi(fnap->arg);
          fnap->argl[fnap->nargs] = fnap->argp +1;
          fnap->bufp = fnap->argl[fnap->nargs]+ fnap->arglen[fnap->nargs];
          if (fnap->bufp - buf > sz) {
             fprintf (stderr, "underrun %ld %d\n", fnap->bufp -buf, sz); 
             return -1;
          }
          fnap->argp=fnap->bufp -1;
          if (*fnap->bufp == ',') {
             if (deb > 3) fprintf(stderr, "g1 bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
            *fnap->bufp = 0;
            fnap->bufp++;
            fnap->argp++;
          } 
          fnap->arg = fnap->bufp;
          fnap->nargs++;
       } else if (*fnap->argp == '\n') {
          if (deb > 3) fprintf(stderr, "g bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
          *fnap->argp = 0;
          if (*fnap->arg) {
             if (deb > 3) fprintf(stderr, "h bufp [%s] argp[%s]\n", fnap->bufp, fnap->argp);
             fnap->argl[fnap->nargs] = fnap->arg;
             fnap->arglen[fnap->nargs] = -1;
             fnap->nargs++;
          }
          fnap->instr = 0;
          fnap->prfx = 0;
          fnap->bufp = fnap->argp +1;
          break;
       } else {
         //do nothing
       }
       fnap->bufp = fnap->argp +1;
     }
     return fnap->bufp - buf;
}

int callfn(fnargs_t * fnap, char * wbuf)
{

     for (int n = 0; n < fnap->nargs; n++) {
        if (fnap->arglen[n] == -1) { 
          if (fnap->argl[n][0] == '"') {
             fnap->argl[n] = fnap->argl[n] + 1;
          }
          int l = strlen(fnap->argl[n]) -1;
          if (fnap->argl[n][l] == '"') fnap->argl[n][l] = 0; 
        }
        if (deb > 2) {
             if (fnap->arglen[n] == -1) fprintf (stderr, "  arg %d [%s] len %d\n", n, fnap->argl[n], fnap->arglen[n]);
             else {
               char argt[10];
               sprintf(argt, "  arg %d", n);
               hexdump(argt, fnap->argl[n], fnap->arglen[n]);
            }
        }
     }
     fcall_t * fnsp = fns;
     while (fnsp->name) {
       if (!strcmp(fnsp->name, fnap->fname)) break;
       fnsp++;
     }
     if (!fnsp->name) {
       fprintf(stderr, "unk fn %s\n", fnap->fname); 
       strcpy(wbuf, "-1\n");
       return 3;
     }
     if (deb > 2) fprintf(stderr, "found %s nargs %d nrets %d\n", fnap->fname, fnap->nargs, fnsp->nrets);
     if (fnap->nargs != fnsp->nargs) {
        fprintf(stderr, "incorrect nargs %d\n", fnap->nargs); 
       	strcpy(wbuf, "-1\n");
        return 3;
     }
     char ** retl = NULL; 
     int * retlen = NULL;
     if (fnsp->nrets) {
       retl = malloc(sizeof(char *)*fnsp->nrets);
       retlen = malloc(sizeof(int) * fnsp->nrets);
       for (int i = 0; i < fnsp->nrets; i++) {
          retl[i] = malloc(20);
          retlen[i] = -1;
       }
     }
     int res = fnsp->function(fnap->nargs, fnap->argl, fnap->arglen, fnsp->nrets, retl, retlen);
     int l = sprintf(wbuf, "%d", res);
     for (int i = 0; i < fnsp->nrets; i++) {
        if (retlen[i] == -1)  l += sprintf(wbuf+l, ",%s", retl[i]);
        else {
          l += sprintf(wbuf + l, ",%d#", retlen[i]); 
          memcpy (wbuf +l, retl[i], retlen[i]);
          l+= retlen[i];
        }        
        free(retl[i]);
     }
     l += sprintf(wbuf+l, "\n");
     if (retl) free (retl);
     if (retlen) free (retlen);
     return l;
}


int main(int argc, char **argv)
{
  int sfifo = -1;
  int cfifo = -1;
  int nr;
  char buf[8192];
  char wbuf[255];
  int sockfd, connfd; 
  fd_set read_fds;
  int nfds;
  struct timeval tv;
  int port = 2020;
  char * addr;
  int  ch;
  
  boardId = defboardId;
  boardnum = defboardnum;
 
  while ((ch = getopt(argc, argv, "b:d:fn:p:")) != -1) {
     switch (ch) {
        case 'b':
           boardnum = atoi(optarg);
           break;
        case 'd':
           deb = atoi(optarg);
           break;
        case 'f':
           fifo = 1;
           break;
        case 'p':
           port = atoi(optarg);
           fifo = 0;
           break;
        case 'n':
           boardId = strdup(optarg); 
           break;
        case '?':
        default:
              fprintf(stderr, "usage %s -f [-p port] [-b boardnum] [-n boardname] [-d dbglvl]", argv[0]); 
        }
  }   
  if (fifo) {
    mkfifo("/tmp/ssiclfifo", S_IRUSR | S_IWUSR); 
    mkfifo("/tmp/csiclfifo", S_IRUSR | S_IWUSR);
  } else {
      struct sockaddr_in servaddr; 
  
      // socket create and verification 
      sockfd = socket(AF_INET, SOCK_STREAM, 0); 
      int reuse = 1;
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse , sizeof(int));
      if (sockfd == -1) { 
          perror("socket creation failed...\n"); 
          exit(0); 
      } 
      else
          if (deb > 1) fprintf(stderr, "Socket successfully created..\n"); 
      bzero(&servaddr, sizeof(servaddr)); 
    
      // assign IP, PORT 
      servaddr.sin_family = AF_INET; 
      servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
      servaddr.sin_port = htons(port); 
    
      // Binding newly created socket to given IP and verification 
      if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
          perror("socket bind failed...\n"); 
          exit(1); 
      } 
      else
          if (deb > 1) fprintf(stderr, "Socket successfully bound..\n"); 
    
      // Now server is ready to listen and verification 
      if ((listen(sockfd, 1)) != 0) { 
          perror("Listen failed...\n"); 
          exit(0); 
      } 
      else
          if (deb > 0) fprintf(stderr, "Server listening..\n"); 
  }
  do {
    if (fifo) {
      sfifo = open("/tmp/ssiclfifo", O_RDONLY);
      cfifo = open("/tmp/csiclfifo", O_WRONLY);
    
      if (sfifo < 0 || cfifo < 0) {
        perror("opening fifo");
      }
    } else {
      struct sockaddr_in cli;
      unsigned socklen = sizeof(cli);

      // Accept the data packet from client and verification 
      connfd = accept(sockfd, (SA*)&cli, &socklen); 
      if (connfd < 0) { 
          perror("server accept failed...\n"); 
          exit(0); 
      } 
      else
          if (deb > 0) fprintf(stderr,"server accepts client...\n"); 
      sfifo = cfifo = connfd;
    }
    do {
     nr = 0;
     do {
       FD_ZERO(&read_fds);
       FD_SET(sfifo, &read_fds);
       tv.tv_sec = 2;
       tv.tv_usec = 0;
       if((nfds = select(sfifo+1, &read_fds, NULL, NULL, &tv)) < 0) {
          perror("select()");
       };
       if (nfds == 0) {
          short srqi;
          if (deb > 2) fprintf(stderr, "testing SRQ..\n");
          TestSRQ (boardnum, &srqi);
          if (srqi) {
             if (deb > 1) fprintf(stderr, "SRQ..\n");
             write(cfifo, "SRQ\n", 4); 
          }
          continue;
       } else { 
          int n = read(sfifo, buf+nr, 8192);
          if (n <= 0) break;
          nr += n;
       }
     } while (buf[nr-1] != '\n'); 
     buf[nr] = 0;
     if (deb > 2) {
        //fprintf(stderr, "read %d [%s]\n", nr, buf); 
        hexdump("read", buf, nr);
     }
     int np = 0;
     while (np < nr){
       fnargs_t * fnap = mkfnargs(10);
       int n;
       while ((n =  parsargs(buf +np, nr -np, fnap)) == -1) {
          int n2 = read (sfifo, buf + nr, 8192 - nr) ;
          nr += n2;

       }
       if (n == 0) {
        fprintf(stderr, "parse failure (no fname)[%s]\n", buf); 
        break;
       }
       np += n;
       if(np < nr) fprintf(stderr, "more messages %d %d [%s]\n", np, nr, buf);
       if (deb > 2) fprintf(stderr, "nargs %d\n", fnap->nargs); 
       int l = callfn(fnap, wbuf);
       if (deb > 2) fprintf(stderr, "result [%s]\n", wbuf);
       write(cfifo, wbuf, l); 
       free(fnap->argl);
       free(fnap->arglen);
       free(fnap);
    }
   } while (nr > 0);
    if (deb > 0) fprintf(stderr, "closing connnection..\n");
    close (cfifo);
    close (sfifo);
  } while (1);
   
}

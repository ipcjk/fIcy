/*
 * fIcy - HTTP/1.0-ICY stream extractor/separator - implementation
 * Copyright(c) 2003-2004 of wave++ (Yuri D'Elia) <wavexx@users.sf.net>
 * Distributed under GNU LGPL without ANY warranty.
 */

// local headers
#include "fIcy.hh"
#include "icy.hh"
#include "http.hh"
#include "hdrparse.hh"
#include "sanitize.hh"
#include "urlparse.hh"
#include "rewrite.hh"
#include "match.hh"
#include "msg.hh"
using std::string;
using std::map;

// system headers
#include <iostream>
using std::cout;
using std::cerr;

#include <fstream>
using std::ofstream;

#include <stdexcept>
using std::runtime_error;

#include <limits>
#include <memory>
using std::auto_ptr;

// c system headers
#include <cstdlib>
#include <sys/types.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


// constants (urgh)
bool dupStdout(true);
char* lastFName(NULL);


void
shwIcyHdr(const map<string, string>& headers, const char* search,
    const char* name)
{
  map<string, string>::const_iterator it;
  if((it = headers.find(search)) != headers.end())
    cerr << name << it->second << std::endl;
}


// this helper function returns a new file opened for writing. errors during
// open or creat are (eventually) thrown as runtime_errors.
ofstream*
newFWrap(const char* file, const bool clobber, const bool ign)
{
  if(!clobber && !access(file, F_OK))
  {
    if(ign)
      return NULL;
    else
      throw runtime_error(string("cannot clobber existing file: ") + file);
  }

  ofstream* out(new ofstream(file, std::ios_base::out));
  if(!*out)
  {
    delete out;
    throw runtime_error(string("cannot write `") + file + "'");
  }

  return out;
}


// a wrapper to the wrapper: if a file exists retry with a new file with
// an incremental number appended. changes file to the real name of the file
ofstream*
newNFWrap(string& file, const bool ign)
{
  ofstream* out(newFWrap(file.c_str(), false, true));
  if(!out)
  {
    char buf[16];
    for(size_t n = 0; n != std::numeric_limits<size_t>::max(); ++n)
    {
      snprintf(buf, sizeof(buf), ".%lu", n);
      if((out = newFWrap((file + buf).c_str(), false, true)))
      {
        file += buf;
        break;
      }
    }

    if(!(out || ign))
      throw runtime_error(string("no free files for `") + file + "'");
  }

  return out;
}


// SIGPIPE handler
void
sigPipe(const int)
{
  msg("broken pipe: disabling duping.");
  dupStdout = false;

  // close the descriptor: this is needed to release the resource
  close(STDOUT_FILENO);
}


// termination handler
void
sigTerm(const int)
{
  if(lastFName)
  {
    msg("removing incomplete last file: %s", lastFName);
    unlink(lastFName);
  }
  exit(Exit::success);
}


// signal installers
void
sigPipeInst()
{
  signal(SIGPIPE, SIG_IGN);
}


void
sigTermInst(const bool handlePipe)
{
  signal(SIGTERM, sigTerm);
  signal(SIGINT, sigTerm);
  signal(SIGHUP, sigTerm);
  if(handlePipe)
    signal(SIGPIPE, sigTerm);
}


// current song status
std::ostream&
hour(std::ostream& fd)
{
  fd.setf(std::ios_base::right);
  fd.width(2);
  fd.fill('0');

  return fd;
}


/*
 * function naming scheme: yep. I'm again in a new coding style internal fight.
 * But this time, I've been seriously tainted by functional/logical programming
 * so be patient. When I'll come up with clear ideas, I will uniform the code.
 */
time_t
display_status(const string& title, const size_t num, const time_t last)
{
  time_t now(time(NULL));

  if(last)
  {
    // show the song duration
    cerr << " [" << hour << ((now - last) / 60) << ":" <<
      hour << ((now - last) % 60) << "]\n";
  }

  cerr << "playing #" << num << ": " << title << std::flush;
  return now;
}


// update the sequence file
void
write_seq(ofstream& out, const char* file)
{
  if(out.rdbuf()->is_open())
  {
    out << file << std::endl;
    if(!out)
      throw runtime_error("cannot append to sequence file");
  }
}


// implementation
int
main(int argc, char* const argv[]) try
{
  // option defaults
  prg = argv[0];

  char* outFile(NULL);
  char* suffix(NULL);
  char* sedFile(NULL);
  ofstream seq;
  bool enuFiles(false);
  bool useMeta(false);
  bool showMeta(false);
  bool clobber(true);
  bool ignFErr(true);
  bool numEFiles(false);
  bool instSignal(false);
  bool rmPartial(false);
  BMatch match;

  int arg;
  while((arg = getopt(argc, argv, "do:emvtcs:inprhq:x:X:f:")) != -1)
    switch(arg)
    {
    case 'd':
      dupStdout = false;
      break;

    case 'o':
      outFile = optarg;
      break;

    case 'e':
      enuFiles = true;
      break;

    case 'm':
      useMeta = true;
      break;

    case 'v':
      verbose = true;
      break;

    case 't':
      showMeta = true;
      break;

    case 'c':
      clobber = false;
      break;

    case 's':
      suffix = optarg;
      break;

    case 'i':
      clobber = false;
      ignFErr = false;
      break;

    case 'n':
      numEFiles = true;
      break;

    case 'p':
      instSignal = true;
      break;

    case 'r':
      rmPartial = true;
      break;

    case 'q':
      seq.open(optarg, std::ios_base::out | std::ios_base::app);
      if(!seq)
      {
	err("cannot open `%s' for appending", optarg);
	return Exit::fail;
      }
      break;

    case 'x':
      match.include(optarg);
      break;

    case 'X':
      match.exclude(optarg);
      break;

    case 'f':
      sedFile = optarg;
      break;

    case 'h':
      cout << prg << fIcy::fIcyHelp << prg << " v" << fIcy::version <<
        " is\n" << fIcy::copyright;

    default:
      return Exit::args;
    }

  argc -= optind;
  if(argc < 1 || argc > 3)
  {
    err("bad parameters; see %s -h", prg);
    return Exit::args;
  }

  // connection parameters
  string proto;
  string server;
  int port;
  string path;
  if(argc > 1)
  {
    server = argv[optind++];
    port = atoi(argv[optind++]);
    path = (argc == 3? argv[optind++]: "/");
  }
  else
  {
    urlParse(proto, server, port, path, argv[optind++]);
    if(proto.size() && proto != "http" && proto != "icy")
    {
      err("unknown protocol \"%s\"", proto.c_str());
      return Exit::args;
    }
  }

  // check for parameters consistency
  // enuFiles and useMeta requires a prefix
  bool reqMeta(enuFiles || useMeta || showMeta);
  if((useMeta || enuFiles) && !outFile)
  {
    err("a prefix is required (see -o) when writing multiple files");
    return Exit::args;
  }

  // you cannot disable duping if you don't write anything!
  if(!(outFile || dupStdout))
  {
    err("trying to perform a do-nothing download");
    return Exit::args;
  }

  // spawn the rewrite coprocess before installing signals
  // TODO: we should probably mask the signal now
  Rewrite rewrite(sedFile);

  // install the signals
  instSignal = instSignal && dupStdout;
  if(instSignal)
    sigPipeInst();
  if(rmPartial && (enuFiles || useMeta))
    sigTermInst(!instSignal);

  // resolve the hostname
  msg("connecting to (%s %d)", server.c_str(), port);
  Http::Http httpc(server.c_str(), port);
    
  // setup headers
  Http::Header qHeaders;
  Http::Header aHeaders;
  Http::Reply reply(&aHeaders);
  
  // query
  qHeaders.push_back(fIcy::userAgent);
  if(reqMeta)
    qHeaders.push_back(ICY::Proto::reqMeta);
  
  msg("requesting stream on (%s)", path.c_str());
  auto_ptr<Socket> s(httpc.get(path.c_str(), reply, &qHeaders));

  // validate the reply
  if(reply.code != Http::Proto::ok)
  {
    err("unexpected reply: %d %s", reply.code,
	sanitize_esc(
	    (reply.description.size()? reply.description: reply.proto)
	    ).c_str());
    return Exit::fail;
  }
  
  map<string, string> pReply(Http::hdrParse(aHeaders));
  if(reqMeta && pReply.find(ICY::Proto::metaint) == pReply.end())
  {
    err("requested metadata, but got nothing.");
    return Exit::fail;
  }
  
  // show some headers
  if(verbose)
  {
    shwIcyHdr(pReply, ICY::Proto::notice1, "Notice: ");
    shwIcyHdr(pReply, ICY::Proto::notice2, "Notice: ");
    shwIcyHdr(pReply, ICY::Proto::title, "Title: ");
    shwIcyHdr(pReply, ICY::Proto::genre, "Genre: ");
    shwIcyHdr(pReply, ICY::Proto::url, "URL: ");
    shwIcyHdr(pReply, ICY::Proto::br, "Bit Rate: ");
  }
  
  // start reading
  ssize_t metaInt(reqMeta?
      atol(pReply.find(ICY::Proto::metaint)->second.c_str()): fIcy::bufSz);
  ICY::Reader reader(*s, fIcy::bufSz);
  size_t enu(0);
  time_t tStamp(0);
  string lastTitle;
  
  // initial file
  auto_ptr<std::ostream> out;
  if(outFile && !(enuFiles || useMeta))
    out.reset(newFWrap(outFile, clobber, false));
  else
    // the first filename is unknown as the metadata block will
    // arrive in the next metaInt bytes
    out.reset();

  for(;;)
  {
    if(dupStdout && !cout)
    {
      // try an empty write to determine if the file is ready
      if(!write(STDOUT_FILENO, NULL, 0))
	cout.clear();
    }
    
    // read the stream
    if(reader.dup(out.get(), metaInt, dupStdout) != metaInt)
    {
      msg("connection terminated");
      break;
    }
    if(!(outFile || dupStdout))
      break;
    
    // read metadata
    if(reqMeta)
    {
      map<string, string> data;
      if(reader.readMeta(data))
      {
	map<string, string>::const_iterator it(
	    data.find(ICY::Proto::mTitle));

	// de-uglify
	string title;
	if(it != data.end())
	{
	  // sanitize immediately, we don't want \n for sed
	  title = sanitize_esc(it->second);
	  rewrite(title);
	}

	if(title.size() && (title != lastTitle))
	{
	  string newFName;
	  
	  if(showMeta)
	    tStamp = display_status(title, enu, tStamp);
	  
	  // skip the first filename generation when discarding partials
	  // or when the title doesn't match
	  if((enuFiles || useMeta) && (enu || !rmPartial) &&
	      match(title.c_str()))
	  {
	    newFName = outFile;

	    if(enuFiles)
	    {
	      char buf[16];
	      snprintf(buf, sizeof(buf), (useMeta? "[%lu] ": "%lu"), enu);
	      newFName += buf;
	    }
	    if(useMeta)
	      newFName += sanitize_file(title);
	    if(suffix)
	      newFName += suffix;
	    
	    // open the new file
	    if(numEFiles)
	      out.reset(newNFWrap(newFName, ignFErr));
	    else
	      out.reset(newFWrap(newFName.c_str(), clobber, ignFErr));
	  }
	  else
	    out.reset();
	  
	  // update the last filename pointer
	  if(lastFName)
	    free(lastFName);
	  if(out.get())
	  {
	    lastFName = strdup(newFName.c_str());
	    write_seq(seq, lastFName);
	    msg("file changed to: %s", lastFName);
	  }
	  else if(lastFName)
	  {
	    lastFName = NULL;
	    msg("file reset");
	  }
	  
	  // update stream number
	  lastTitle = title;
	  ++enu;
	}
      }
    }
  }

  return Exit::success;
}
catch(runtime_error& err)
{
  ::err("%s", err.what());
  return Exit::fail;
}
catch(...)
{
  err("unknown error, aborting");
  abort();
}

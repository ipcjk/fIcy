/*
 * htFollow - follow an http url until content is reached - implementation
 * Copyright(c) 2005-2015 of wave++ (Yuri D'Elia)
 * Distributed under GNU LGPL without ANY warranty.
 */

// interface and local headers
#include "htfollow.hh"
#include "hdrparse.hh"
#include "sanitize.hh"
#include "msg.hh"
using std::string;
using std::map;

// system headers
#include <memory>
using std::auto_ptr;

#include <stdexcept>
using std::runtime_error;

// c system headers
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>


// implementation
string
itos(const int i)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", i);
  return string(buf);
}


Socket*
htFollow(map<string, string>& pReply, const URL& url, const Http::Header qHeaders,
    size_t limit, time_t timeout, size_t retries, size_t waitSecs)
{
  URL buf = url;
  timeval tmBuf;
  if(timeout)
  {
    tmBuf.tv_sec = timeout;
    tmBuf.tv_usec = 0;
  }

  // connection loop
  auto_ptr<Socket> s;
  for(size_t level = limit, retry = retries;;)
  {
    // display the correct port name/number
    if(!buf.port.size())
      buf.port = Http::Proto::port;

    msg("connecting to %s:%s", sanitize_esc(buf.server).c_str(), buf.port.c_str());
    Http::Http httpc(buf.server.c_str(), buf.port.c_str(), (timeout? &tmBuf: NULL));

    msg("requesting data on %s", sanitize_esc(buf.path).c_str());
    Http::Header aHeaders;
    Http::Reply reply(&aHeaders);
    try
    {
      Socket* tmp = httpc.get(buf.path.c_str(), reply, &qHeaders);
      s.reset(tmp);
    }
    catch(runtime_error& err)
    {
      if(!retry--)
	throw err;
      else
      {
	msg("request failure: %s", err.what());
	sleep(waitSecs);
	continue;
      }
    }

    // validate the reply code
    if(reply.code != Http::Proto::ok &&
	reply.code != Http::Proto::moved &&
	reply.code != Http::Proto::found &&
	reply.code != Http::Proto::other)
      throw runtime_error(string("unexpected reply: ") +
	  itos(reply.code) + " " + sanitize_esc((reply.description.size()?
		  reply.description: reply.proto)).c_str());

    // parse the headers
    pReply = Http::hdrParse(aHeaders);
    if(reply.code == Http::Proto::ok)
      break;

    // recursion
    if(!level--)
      throw runtime_error(string("hit redirect follow limit: ") + itos(limit));

    map<string, string>::iterator urlPos = pReply.find(Http::Proto::location);
    if(urlPos == pReply.end())
      throw runtime_error("redirection didn't contain an url");
    if(reply.code == Http::Proto::moved)
      err("warning: content moved permanently to %s",
	  sanitize_esc(urlPos->second).c_str());

    buf = urlPos->second;
    if(buf.proto.size() && buf.proto != url.proto)
      throw runtime_error(
	  string("protocol changes are not allowed in redirection (") +
	  url.proto + " -> " + sanitize_esc(buf.proto) + ")");
  }

  return s.release();
}

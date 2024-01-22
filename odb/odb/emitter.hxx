// file      : odb/emitter.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_EMITTER_HXX
#define ODB_EMITTER_HXX

#include <string>
#include <sstream>

// Emit a code construct one line at a time.
//
struct emitter
{
  virtual void
  pre ();

  virtual void
  line (const std::string&) = 0;

  virtual void
  post ();
};

// Send output line-by-line (std::endl marker) to the emitter.
//
class emitter_ostream: public std::ostream
{
public:
  emitter_ostream (emitter& e): std::ostream (&buf_), buf_ (e) {}

private:
  class streambuf: public std::stringbuf
  {
  public:
    streambuf (emitter& e): e_ (e) {}

    virtual int
    sync ();

  private:
    emitter& e_;
  };

  streambuf buf_;
};

#endif // ODB_EMITTER_HXX

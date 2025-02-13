//    This is part of the iostream library, providing -*- C++ -*- input/output.
//    Copyright (C) 1991 Per Bothner.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public


#ifndef _STREAMBUF_H
#define _STREAMBUF_H
#ifdef __GNUG__
#pragma interface
#endif

#include <_G_config.h>
#ifdef _G_NEED_STDARG_H
#include <stdarg.h>
#endif
#ifndef fpos_t
#define fpos_t _G_fpos_t
#endif

#ifndef EOF
#define EOF (-1)
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif

class ostream; class streambuf; class backupbuf;

#ifdef _G_FRIEND_BUG
extern int __UNDERFLOW(streambuf*);
extern int __OVERFLOW(streambuf*, int);
#endif
extern "C" int __underflow(streambuf*);
extern "C" int __overflow(streambuf*, int);

typedef _G_off_t streamoff;
typedef _G_off_t streampos; // Should perhaps be _G_fpos_t ?

typedef unsigned long __fmtflags;
typedef unsigned char __iostate;

struct _ios_fields { // The data members of an ios.
    streambuf *_strbuf;
    ostream* _tie;
    int _width;
    __fmtflags _flags;
    char _fill;
    __iostate _state;
    unsigned short _precision;
};

#define _IOS_GOOD	0
#define _IOS_EOF	1
#define _IOS_FAIL	2
#define _IOS_BAD	4

#define _IOS_INPUT	1
#define _IOS_OUTPUT	2
#define _IOS_ATEND	4
#define _IOS_APPEND	8
#define _IOS_TRUNC	16
#define _IOS_NOCREATE	32
#define _IOS_NOREPLACE	64

#ifdef _STREAM_COMPAT
enum state_value {
    _good = _IOS_GOOD,
    _eof = _IOS_EOF,
    _fail = _IOS_FAIL,
    _bad = _IOS_BAD };
enum open_mode {
    input = _IOS_INPUT,
    output = _IOS_OUTPUT,
    atend = _IOS_ATEND,
    append = _IOS_APPEND };
#endif

class ios : public _ios_fields {
  public:
    typedef __fmtflags fmtflags;
    typedef __iostate iostate;
    enum io_state {
	goodbit = _IOS_GOOD,
	eofbit = _IOS_EOF,
	failbit = _IOS_FAIL,
	badbit = _IOS_BAD };
    enum open_mode {
	in = _IOS_INPUT,
	out = _IOS_OUTPUT,
	ate = _IOS_ATEND,
	app = _IOS_APPEND,
	trunc = _IOS_TRUNC,
	nocreate = _IOS_NOCREATE,
	noreplace = _IOS_NOREPLACE };
    enum seek_dir { beg, cur, end};
    enum { skipws=01, left=02, right=04, internal=010,
	   dec=020, oct=040, hex=0100,
	   showbase=0200, showpoint=0400, uppercase=01000, showpos=02000,
	   scientific=04000, fixed=0100000, unitbuf=020000, stdio=040000,
	   dont_close=0x80000000 //Don't delete streambuf on stream destruction
	   };

    ostream* tie() const { return _tie; }
    ostream* tie(ostream* val) { ostream* save=_tie; _tie=val; return save; }

    // Methods to change the format state.
    char fill() const { return _fill; }
    char fill(char newf) { char oldf = _fill; _fill = newf; return oldf; }
    fmtflags flags() const { return _flags; }
    fmtflags flags(fmtflags new_val) {
	fmtflags old_val = _flags; _flags = new_val; return old_val; }
    int precision() const { return _precision; }
    int precision(int newp) {
	unsigned short oldp = _precision; _precision = (unsigned short)newp;
	return oldp; }
    fmtflags setf(fmtflags val) {
	fmtflags oldbits = _flags;
	_flags |= val; return oldbits; }
    fmtflags setf(fmtflags val, fmtflags mask) {
	fmtflags oldbits = _flags;
	_flags = (_flags & ~mask) | (val & mask); return oldbits; }
    fmtflags unsetf(fmtflags mask) {
	fmtflags oldbits = _flags & mask;
	_flags &= ~mask; return oldbits; }
    int width() const { return _width; }
    int width(int val) { int save = _width; _width = val; return save; }

    static const unsigned long basefield;
    static const unsigned long adjustfield;
    static const unsigned long floatfield;

    streambuf* rdbuf() const { return _strbuf; }
    void clear(iostate state = 0) { _state = state; }
    void set(iostate flag) { _state |= flag; } // In ANSI but not AT&T.
    int good() const { return _state == 0; }
    int eof() const { return _state & ios::eofbit; }
    int fail() const { return _state & (ios::badbit|ios::failbit); }
    int bad() const { return _state & ios::badbit; }
    int rdstate() const { return _state; }
    operator void*() const { return fail() ? (void*)0 : (void*)(-1); }
    int operator!() const { return fail(); }

#ifdef _STREAM_COMPAT
    void unset(state_value flag) { _state &= ~flag; }
    void close();
    int is_open();
    int readable();
    int writable();
#endif

  protected:
    ios(streambuf* sb = 0, ostream* tie = 0);
    ~ios();
};

#if __GNUG__==1
typedef int _seek_dir;
#else
typedef ios::seek_dir _seek_dir;
#endif

// Magic numbers and bits for the _flags field.
// The magic numbers use the high-order bits of _flags;
// the remaining bits are abailable for variable flags.
// Note: The magic numbers must all be negative if stdio
// emulation is desired.

#define _IO_MAGIC 0xFBAD0000 /* Magic number */
#define _OLD_STDIO_MAGIC 0xFABC0000 /* Emulate old stdio. */
#define _IO_MAGIC_MASK 0xFFFF0000
#define _S_USER_BUF 1 /* User owns buffer; don't delete it on close. */
#define _S_UNBUFFERED 2
#define _S_NO_READS 4 /* Reading not allowed */
#define _S_NO_WRITES 8 /* Writing not allowd */
#define _S_EOF_SEEN 0x10
#define _S_ERR_SEEN 0x20
#define _S_DELETE_DONT_CLOSE 0x40
#define _S_LINKED 0x80 // Set if linked (using _chain) to streambuf::_list_all.
#define _S_IN_BACKUP 0x100
#define _S_LINE_BUF 0x200
#define _S_TIED_PUT_GET 0x400 // Set if put and get pointer logicly tied.
#define _S_CURRENTLY_PUTTING 0x800
#define _S_IS_BACKUPBUF 0x4000
#define _S_IS_FILEBUF 0x8000

// A streammarker remembers a position in a buffer.
// You are guaranteed to be able to seek back to it if it is saving().
class streammarker {
    friend class streambuf;
#ifdef _G_FRIEND_BUG
    friend int __UNDERFLOW(streambuf*);
#else
    friend int __underflow(streambuf*);
#endif
    struct streammarker *_next;  // Only if saving()
    streambuf *_sbuf; // Only valid if saving().
    streampos _spos; // -2: means that _pos is valid.
    void set_streampos(streampos sp) { _spos = sp; }
    void set_offset(int offset) { _pos = offset; _spos = (streampos)(-2); }
    // If _pos >= 0, it points to _buf->Gbase()+_pos.
    // if _pos < 0, it points to _buf->eBptr()+_pos.
    int _pos;
  public:
    streammarker(streambuf *sb);
    ~streammarker();
    int saving() { return  _spos == -2; }
    int delta(streammarker&);
    int delta();
};

struct __streambuf {
    // NOTE: If this is changed, also change __FILE in stdio/stdio.h!
    int _flags;		/* High-order word is _IO_MAGIC; rest is flags. */
    char* _gptr;	/* Current get pointer */
    char* _egptr;	/* End of get area. */
    char* _eback;	/* Start of putback+get area. */
    char* _pbase;	/* Start of put area. */
    char* _pptr;	/* Current put pointer. */
    char* _epptr;	/* End of put area. */
    char* _base;	/* Start of reserve area. */
    char* _ebuf;	/* End of reserve area. */
    struct streambuf *_chain;

    // The following fields are used to support backing up and undo.
    friend class streammarker;
    char *_other_gbase; // Pointer to start of non-current get area.
    char *_aux_limit;  // Pointer to first valid character of backup area,
    char *_other_egptr; // Pointer to end of non-current get area.
    streammarker *_markers;

#define __HAVE_COLUMN /* temporary */
    // 1+column number of pbase(); 0 is unknown.
    unsigned short _cur_column;
    char _unused;
    char _shortbuf[1];
};

extern unsigned __adjust_column(unsigned start, const char *line, int count);

struct streambuf : private __streambuf {
    friend class ios;
    friend class istream;
    friend class ostream;
    friend class streammarker;
#ifdef _G_FRIEND_BUG
    friend int __UNDERFLOW(streambuf*);
#else
    friend int __underflow(streambuf*);
#endif
  protected:
    static streambuf* _list_all; /* List of open streambufs. */
    streambuf*& xchain() { return _chain; }
    void _un_link();
    void _link_in();
    char* gptr() const { return _gptr; }
    char* pptr() const { return _pptr; }
    char* egptr() const { return _egptr; }
    char* epptr() const { return _epptr; }
    char* pbase() const { return _pbase; }
    char* eback() const { return _eback; }
    char* ebuf() const { return _ebuf; }
    char* base() const { return _base; }
    void xput_char(char c) { *_pptr++ = c; }
    int xflags() { return _flags; }
    int xflags(int f) { int fl = _flags; _flags = f; return fl; }
    void xsetflags(int f) { _flags |= f; }
    void xsetflags(int f, int mask) { _flags = (_flags & ~mask) | (f & mask); }
    void gbump(int n) { _gptr += n; }
    void pbump(int n) { _pptr += n; }
    void setb(char* b, char* eb, int a=0);
    void setp(char* p, char* ep) { _pbase=_pptr=p; _epptr=ep; }
    void setg(char* eb, char* g, char *eg) { _eback=eb; _gptr=g; _egptr=eg; }
    char *shortbuf() { return _shortbuf; }

    int in_backup() { return _flags & _S_IN_BACKUP; }
    // The start of the main get area:  FIXME:  wrong for write-mode filebuf?
    char *Gbase() { return in_backup() ? _other_gbase : _eback; }
    // The end of the main get area:
    char *eGptr() { return in_backup() ? _other_egptr : _egptr; }
    // The start of the backup area:
    char *Bbase() { return in_backup() ? _eback : _other_gbase; }
    char *Bptr() { return _aux_limit; }
    // The end of the backup area:
    char *eBptr() { return in_backup() ? _egptr : _other_egptr; }
    char *Nbase() { return _other_gbase; }
    char *eNptr() { return _other_egptr; }
    int have_backup() { return _other_gbase != NULL; }
    int have_markers() { return _markers != NULL; }
    int _least_marker();
    void switch_to_main_get_area();
    void switch_to_backup_area();
    void free_backup_area();
    void unsave_markers(); // Make all streammarkers !saving().
    int put_mode() { return _flags & _S_CURRENTLY_PUTTING; }
    int switch_to_get_mode();
    
    streambuf(int flags=0);
  public:
    static int flush_all();
    static void flush_all_linebuffered(); // Flush all line buffered files.
    virtual int underflow() = 0; // Leave public for now
    virtual int overflow(int c = EOF) = 0; // Leave public for now
    virtual int doallocate();
    virtual streampos seekoff(streamoff, _seek_dir, int mode=ios::in|ios::out);
    virtual streampos seekpos(streampos pos, int mode = ios::in|ios::out);
    int seekmark(streammarker& mark, int delta = 0);
    int sputbackc(char c);
    int sungetc();
    virtual ~streambuf();
    int unbuffered() { return _flags & _S_UNBUFFERED ? 1 : 0; }
    int linebuffered() { return _flags & _S_LINE_BUF ? 1 : 0; }
    void unbuffered(int i)
	{ if (i) _flags |= _S_UNBUFFERED; else _flags &= ~_S_UNBUFFERED; }
    void linebuffered(int i)
	{ if (i) _flags |= _S_LINE_BUF; else _flags &= ~_S_LINE_BUF; }
    int allocate() { // For AT&T compatibility
	if (base() || unbuffered()) return 0;
	else return doallocate(); }
    // Allocate a buffer if needed; use _shortbuf if appropriate.
    void allocbuf() { if (base() == NULL) doallocbuf(); }
    void doallocbuf();
    virtual int sync();
    virtual int pbackfail(int c);
    virtual int ungetfail();
    virtual streambuf* setbuf(char* p, int len);
    int in_avail() { return _egptr - _gptr; }
    int out_waiting() { return _pptr - _pbase; }
    virtual int sputn(const char* s, int n);
    int padn(char pad, int n); // Emit 'n' copies of 'pad'.
    virtual int sgetn(char* s, int n);
    int ignore(int);
    virtual int get_column();
    virtual int set_column(int);
    long sgetline(char* buf, _G_size_t n, char delim, int putback_delim);
    int sbumpc() {
	if (_gptr >= _egptr && __underflow(this) == EOF) return EOF;
	else return *(unsigned char*)_gptr++; }
    int sgetc() {
	if (_gptr >= _egptr && __underflow(this) == EOF) return EOF;
	else return *(unsigned char*)_gptr; }
    int snextc() {
	if (++_gptr >= _egptr && __underflow(this) == EOF) return EOF;
	else return *(unsigned char*)_gptr; }
    int sputc(int c) {
	if (_pptr >= _epptr) return __overflow(this, (unsigned char)c);
	return *_pptr++ = c, (unsigned char)c; }
    void stossc() { if (_gptr < _egptr) _gptr++; }
    int vscan(char const *fmt0, _G_va_list ap, ios::iostate* = NULL);
    int vform(char const *fmt0, _G_va_list ap);
#if 0 /* Work in progress */
    int collumn();  // Current collumn number (of put pointer). -1 is unknown.
    void collumn(int c);  // Set collumn number of put pointer to c.
#endif
};

// A backupbuf is a streambuf with full backup and savepoints on reading.
// All standard streambufs in the GNU iostream library are backupbufs.

// A backupbuf may have two get area:
// - The main get area, and (sometimes) the putback area.
// Whichever one of these contains the gptr is the current get area;
// the other one is the non-current get area.

class backupbuf : public streambuf {
    friend class streammarker;
  protected:
    backupbuf(int flags=0) : streambuf(flags|_S_IS_BACKUPBUF) { }
  public:
    virtual int pbackfail(int c);
    virtual int underflow();
    virtual int overflow(int c = EOF);
};

struct __file_fields {
    short _fileno;
    int _blksize;
    fpos_t _offset;
//    char* _save_gptr;  char* _save_egptr;
};

class filebuf : public backupbuf {
  protected:
    struct __file_fields _fb;
    void init();
  public:
    filebuf();
    filebuf(int fd);
    filebuf(int fd, char* p, int len);
    ~filebuf();
    filebuf* attach(int fd);
    filebuf* open(const char *filename, const char *mode);
    filebuf* open(const char *filename, int mode, int prot = 0664);
    virtual int underflow();
    virtual int overflow(int c = EOF);
    int is_open() { return _fb._fileno >= 0; }
    int fd() { return is_open() ? _fb._fileno : EOF; }
    filebuf* close();
    virtual int doallocate();
    virtual streampos seekoff(streamoff, _seek_dir, int mode=ios::in|ios::out);
    int sputn(const char* s, int n);
    int sgetn(char* s, int n);
    virtual int sync();
    virtual streambuf* setbuf(char* p, int len);
  protected: // See documentation in filebuf.C.
//    virtual int pbackfail(int c);
    int is_reading() { return eback() != egptr(); }
    char* cur_ptr() { return is_reading() ?  gptr() : pptr(); }
    /* System's idea of pointer */
    char* file_ptr() { return eGptr(); }
    int do_write(const char *data, int to_do);
    int do_flush() { return do_write(_pbase, _pptr-_pbase); }
    // Low-level operations (Usually invoke system calls.)
    virtual _G_ssize_t sys_read(char* buf, _G_size_t size);
    virtual fpos_t sys_seek(fpos_t, _seek_dir);
    virtual _G_ssize_t sys_write(const void*, long);
    virtual int sys_stat(void*); // Actually, a (struct stat*)
    virtual int sys_close();
};

#ifdef _STREAM_COMPAT
inline int ios::readable() { return !(rdbuf()->_flags & _S_NO_READS); }
inline int ios::writable() { return !(rdbuf()->_flags & _S_NO_WRITES); }
inline int ios::is_open() { return (rdbuf()->_flags & _S_NO_READS+_S_NO_WRITES)
				!= _S_NO_READS+_S_NO_WRITES; }
#endif
inline ios::ios(streambuf* sb /* = 0 */, ostream* tie /* = 0 */) {
		_tie = tie; _strbuf=sb; _state=0; _width=0; _fill=' ';
		_flags=ios::skipws; _precision=6; }
inline ios::~ios() {
    if (!(_flags & (unsigned int)ios::dont_close)) delete _strbuf; }

#endif /* _STREAMBUF_H */

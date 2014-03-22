#pragma once

#include <muduo/net/Buffer.h>
#include <boost/noncopyable.hpp>
#include <zlib.h>

namespace muduo
{
namespace net
{

class GzipOutputStream : boost::noncopyable
{
 public:
  explicit GzipOutputStream(Buffer* output)
    : output_(output),
      zerror_(Z_OK),
      bufferSize_(1024)
  {
    bzero(&zstream_, sizeof zstream_);
    zerror_ = deflateInit(&zstream_, Z_DEFAULT_COMPRESSION);
  }

  ~GzipOutputStream()
  {
    finish();
  }


  // Return last error message or NULL if no error.
  const char* zlibErrorMessage() const
  {
    return zstream_.msg;
  }

  int zlibErrorCode() const
  {
    return zerror_;
  }

  int internalOutputBufferSize() const
  {
    return bufferSize_;
  }

  bool write(StringPiece buf)
  {
    if (zerror_ != Z_OK)
      return false;

    assert(zstream_.next_in == NULL && zstream_.avail_in == 0);
    void* in = const_cast<char*>(buf.data());
    zstream_.next_in = static_cast<Bytef*>(in);
    zstream_.avail_in = buf.size();
    do
    {
      zerror_ = compress(Z_NO_FLUSH);
    } while (zstream_.avail_in > 0 && zerror_ == Z_OK);
    if (zstream_.avail_in == 0)
    {
      assert(static_cast<const void*>(zstream_.next_in) == buf.end());
      zstream_.next_in = NULL;
    }
    return zerror_ == Z_OK;
  }

  bool finish()
  {
    if (zerror_ != Z_OK)
      return false;

    do
    {
      zerror_ = compress(Z_FINISH);
    } while (zerror_ == Z_OK);
    zerror_ = deflateEnd(&zstream_);
    bool ok = zerror_ == Z_OK;
    zerror_ = Z_STREAM_END;
    return ok;
  }

 private:
  int compress(int flush)
  {
    output_->ensureWritableBytes(bufferSize_);
    zstream_.next_out = reinterpret_cast<Bytef*>(output_->beginWrite());
    zstream_.avail_out = static_cast<int>(output_->writableBytes());
    int error = ::deflate(&zstream_, flush);
    output_->hasWritten(output_->writableBytes() - zstream_.avail_out);
    if (output_->writableBytes() == 0)
    {
      bufferSize_ *= 2;
    }
    return error;
  }

  Buffer* output_;
  z_stream zstream_;
  int zerror_;
  int bufferSize_;
};

}
}

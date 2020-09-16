#ifndef MBUSPARSER_H
#define MBUSPARSER_H

#include <vector>
#include <cstring>
#include <cmath>
#include <cstdint>

struct VectorView {
  VectorView(const std::vector<uint8_t>& data, size_t position, size_t size)
    : m_start(&data[position])
    , m_size(size)
  {
  }
  VectorView(const uint8_t* data, size_t size)
    : m_start(data)
    , m_size(size)
  {
  }
  const uint8_t& operator[](size_t pos) const { return m_start[pos]; }
  const uint8_t& front() const { return m_start[0]; }
  const uint8_t& back() const { return m_start[m_size-1]; }
  size_t size() const noexcept { return m_size; }
  size_t find(const std::vector<uint8_t>& needle) const;
private:
  const uint8_t* m_start;
  size_t m_size;
};

struct MbusStreamParser {
  MbusStreamParser(uint8_t * buff, size_t bufsize);
  bool pushData(uint8_t data);
  const VectorView& getFrame();
  enum BufferContent {
    COMPLETE_FRAME,
    TRASH_DATA
  };
  BufferContent getContentType() const;
private:
  uint8_t* m_buf;
  size_t m_bufsize;
  size_t m_position = 0;

  enum ParseState {
    LOOKING_FOR_START,
    LOOKING_FOR_FORMAT_TYPE,
    LOOKING_FOR_SIZE,
    LOOKING_FOR_END,
  };
  ParseState m_parseState = LOOKING_FOR_START;
  uint16_t m_messageSize = 0;
  VectorView m_frameFound;
  BufferContent m_bufferContent = TRASH_DATA;
};

#endif

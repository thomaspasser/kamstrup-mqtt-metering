#include "mbusparser.h"
#include <cassert>

size_t VectorView::find(const std::vector<uint8_t>& needle) const
{
  for (const uint8_t* it = m_start; it < (m_start+m_size-needle.size()); ++it) {
    if (memcmp(it, &needle[0], needle.size()) == 0) {
      return it-m_start;
    }
  }
  return -1;
}

MbusStreamParser::MbusStreamParser(uint8_t* buf, size_t bufsize)
  : m_buf(buf)
  , m_bufsize(bufsize)
  , m_frameFound(nullptr, 0)
{
}

bool MbusStreamParser::pushData(uint8_t data)
{
  if (m_position >= m_bufsize) {
    // Reached end of buffer
    m_position = 0;
    m_parseState = LOOKING_FOR_START;
    m_messageSize = 0;
    m_bufferContent = TRASH_DATA;
    m_frameFound = VectorView(m_buf, m_bufsize);
    return true;
  }
  switch (m_parseState) {
  case LOOKING_FOR_START:
    m_buf[m_position++] = data;
    if (data == 0x7E) {
      // std::cout << "Found frame start. Pos=" << m_position << std::endl;
      m_parseState = LOOKING_FOR_FORMAT_TYPE;
    }
    break;
  case LOOKING_FOR_FORMAT_TYPE:
    m_buf[m_position++] = data;
    assert(m_position > 1);
    if ((data & 0xF0) == 0xA0) {
      // std::cout << "Found frame format type (pos=" << m_position << "): " << std::hex << (unsigned)data << std::dec << std::endl;
      m_parseState = LOOKING_FOR_SIZE;
      if (m_position-2 > 0) {
        m_bufferContent = TRASH_DATA;
        m_frameFound = VectorView(m_buf, m_position-2);
        return true;
      }
    } else if (data == 0x7E) {
      // std::cout << "Found frame start instead of format type: " << std::hex << (unsigned)data << std::dec << std::endl;
      m_bufferContent = TRASH_DATA;
      m_frameFound = VectorView(m_buf, m_position-1);
      return true;
    } else {
      m_parseState = LOOKING_FOR_START;
    }
    break;
  case LOOKING_FOR_SIZE:
    assert(m_position > 0);
    // Move start of frame to start of buffer
    m_buf[0] = 0x7E;
    m_buf[1] = m_buf[m_position-1];
    m_messageSize = ((m_buf[m_position-1] & 0x0F) << 8) | data;
    // std::cout << "Message size is: " << m_messageSize << ". now looking for end" << std::endl;
    m_buf[2] = data;
    m_position = 3;
    m_parseState = LOOKING_FOR_END;
    break;
  case LOOKING_FOR_END:
    m_buf[m_position++] = data;
    if (m_position == (m_messageSize+2)) {
      if (data == 0x7E) {
        m_bufferContent = COMPLETE_FRAME;
        m_frameFound = VectorView(m_buf, m_position);
        m_parseState = LOOKING_FOR_START;
        m_position = 0;
        // std::cout << "Found end. Returning complete frame. Setting position=0" << std::endl;
        return true;
      } else {
        // std::cout << "Unexpected byte at end position: " << std::hex << (unsigned)data << std::dec << std::endl;
        m_parseState = LOOKING_FOR_START;
      }
    }
    break;
  }
  return false;
}

MbusStreamParser::BufferContent MbusStreamParser::getContentType() const
{
  return m_bufferContent;
}

const VectorView& MbusStreamParser::getFrame()
{
  return m_frameFound;
}

#include <algorithm>

#include "CircularBuffer.h"

CircularBuffer::CircularBuffer(MemorySize blockSize, bool growable)
    : kBlockSize(blockSize.bytes()), kGrowable(growable) {
  init_table();
}

CircularBuffer::~CircularBuffer() { cleanup_table(); }

CircularBuffer::CircularBuffer(const CircularBuffer& other)
    : kBlockSize(other.kBlockSize), kGrowable(other.kGrowable) {
  if (this != &other) {
    copy(other);
  }
}

CircularBuffer& CircularBuffer::operator=(
    const CircularBuffer& other) {
  if (this != &other) {
    // Copy the other.
    const_cast<size_t&>(kBlockSize) = other.kBlockSize;
    const_cast<bool&>(kGrowable) = other.kGrowable;
    copy(other);
  }

  return *this;
}

CircularBuffer::CircularBuffer(CircularBuffer&& other) noexcept
    : kBlockSize(other.kBlockSize), kGrowable(other.kGrowable) {
  if (this != &other) {
    // Copy variables.
    _totalSize = other._totalSize;
    _totalCapacity = other._totalCapacity;
    _hd = other._hd;
    _tl = other._tl;
    _tableCapacity = other._tableCapacity;
    _table = other._table;

    // Invalidate the other.
    other._totalSize = 0;
    other._totalCapacity = 0;
    other._hd = {};
    other._tl = {};

    other._tableCapacity = 0;
    other._table = nullptr;
  }
}

CircularBuffer& CircularBuffer::operator=(
    CircularBuffer&& other) noexcept {
  if (this != &other) {
    // Clean up current table.
    cleanup_table();

    // Copy variables.
    const_cast<size_t&>(kBlockSize) = other.kBlockSize;
    const_cast<bool&>(kGrowable) = other.kGrowable;
    _totalSize = other._totalSize;
    _totalCapacity = other._totalCapacity;
    _hd = other._hd;
    _tl = other._tl;
    _tableCapacity = other._tableCapacity;
    _table = other._table;

    // Invalidate the other.
    other._totalSize = 0;
    other._totalCapacity = 0;
    other._hd = {};
    other._tl = {};

    other._tableCapacity = 0;
    other._table = nullptr;
  }
  return *this;
}

bool CircularBuffer::write(const void* p, size_t size) {
  if (size == 0) return false;

  const uint8_t* src = reinterpret_cast<const uint8_t*>(p);

  // Check overflow.
  if (size > _totalCapacity - _totalSize) {
    if (!kGrowable) return false;

    while (size > _totalCapacity - _totalSize) {
      increase_table();
    }
  }

  // Copy the src data into the table.
  size_t bytes_copied = 0;
  while (bytes_copied < size) {
    size_t in_block = kBlockSize - _hd.m;
    size_t bytes_to_copy = std::min(in_block, size - bytes_copied);

    memcpy(&_table[_hd.n][_hd.m], src + bytes_copied, bytes_to_copy);

    bytes_copied += bytes_to_copy;
    _hd = advance_buf_pointer(_hd, bytes_copied);
  }

  _totalSize += size;

  return true;
}

bool CircularBuffer::read(void* p, size_t size) {
  if (size == 0) return false;

  uint8_t* dst = reinterpret_cast<uint8_t*>(p);

  // Check overflow.
  if (_totalSize < size) {
    return false;
  }

  // Copy the table data to the user dst.
  size_t bytes_copied = 0;
  while (bytes_copied < size) {
    size_t in_block = kBlockSize - _tl.m;
    size_t bytes_to_copy = std::min(in_block, size - bytes_copied);

    memcpy(dst + bytes_copied, &_table[_tl.n][_tl.m], bytes_to_copy);

    bytes_copied += bytes_to_copy;
    _tl = advance_buf_pointer(_tl, bytes_copied);
  }

  _totalSize -= size;

  // Decrease the table by 2 if usage rate is under 1/4.
  double usage_rate = (double)_totalSize / _totalCapacity;
  while (_tableCapacity > 1 && usage_rate < 0.25) {
    decrease_table();
    usage_rate = (double)_totalSize / _totalCapacity;
  }

  return true;
}

bool CircularBuffer::peek(void* p, size_t size) const {
  if (size == 0) return false;

  uint8_t* dst = reinterpret_cast<uint8_t*>(p);

  if (_totalSize < size) {
    return false;
  }

  // Copy the table data to the user dst.
  buf_pointer tmp_tl = _tl;
  size_t bytes_copied = 0;
  while (bytes_copied < size) {
    size_t in_block = kBlockSize - tmp_tl.m;
    size_t bytes_to_copy = std::min(in_block, size - bytes_copied);

    memcpy(dst + bytes_copied, &_table[_tl.n][_tl.m], bytes_to_copy);

    bytes_copied += bytes_to_copy;
    tmp_tl = advance_buf_pointer(tmp_tl, bytes_copied);
  }

  return true;
}

bool CircularBuffer::empty() const { return _totalSize == 0; }

size_t CircularBuffer::size() const { return _totalSize; }

size_t CircularBuffer::capacity() const { return _totalCapacity; }

void CircularBuffer::init_table() {
  _totalSize = 0;
  _totalCapacity = kBlockSize;
  _hd = {};
  _tl = {};
  _tableCapacity = 1;
  _table = (uint8_t**)malloc(_tableCapacity * sizeof(uint8_t*));
  _table[0] = (uint8_t*)malloc(kBlockSize);
}

void CircularBuffer::cleanup_table() {
  for (size_t i = 0; i < _tableCapacity; ++i) {
    free(_table[i]);
  }
  free(_table);

  _totalSize = 0;
  _totalCapacity = 0;
  _hd = {};
  _tl = {};

  _tableCapacity = 0;
  _table = nullptr;
}

void CircularBuffer::copy(const CircularBuffer& other) {
  size_t prevTableCapacity = _tableCapacity;

  // Copy variables.
  _totalSize = other._totalSize;
  _totalCapacity = other._totalCapacity;
  _hd = other._hd;
  _tl = other._tl;
  _tableCapacity = other._tableCapacity;

  // Initialize table if table size is different.
  if (prevTableCapacity != _tableCapacity) {
    // Clean up the previous table.
    for (size_t i = 0; i < prevTableCapacity; ++i) {
      free(_table[i]);
    }
    free(_table);

    // Create a new table.
    _table = (uint8_t**)malloc(_tableCapacity * sizeof(uint8_t*));
    for (size_t i = 0; i < _tableCapacity; ++i) {
      _table[i] = (uint8_t*)malloc(kBlockSize);
    }
  }

  // Copy the table data to the user dst.
  buf_pointer tmp_tl = _tl;
  size_t bytes_copied = 0;
  while (bytes_copied < _totalSize) {
    size_t in_block = kBlockSize - tmp_tl.m;
    size_t bytes_to_copy = std::min(in_block, _totalSize - bytes_copied);

    memcpy(&_table[tmp_tl.n][tmp_tl.m], &other._table[tmp_tl.n][tmp_tl.m],
           bytes_to_copy);

    bytes_copied += bytes_to_copy;
    tmp_tl = advance_buf_pointer(tmp_tl, bytes_copied);
  }
}

void CircularBuffer::increase_table() {
  // 1, 2, 4, 8, ...
  // Create a new memory table.
  size_t new_table_cap = _tableCapacity << 1;
  uint8_t** new_table = (uint8_t**)malloc(new_table_cap * sizeof(uint8_t*));
  for (size_t i = 0; i < new_table_cap; ++i) {
    new_table[i] = (uint8_t*)malloc(kBlockSize);
  }

  // Copy the previous table.
  size_t bytes_copied = 0;
  buf_pointer new_hd{0, 0};
  while (bytes_copied < _totalSize) {
    size_t in_block = kBlockSize - _tl.m;
    size_t bytes_to_copy = std::min(in_block, _totalSize - bytes_copied);

    memcpy(&new_table[new_hd.n][new_hd.m], &_table[_tl.n][_tl.m],
           bytes_to_copy);

    bytes_copied += bytes_to_copy;
    _tl = advance_buf_pointer(_tl, bytes_to_copy);
    new_hd = advance_buf_pointer(new_hd, bytes_to_copy);
  }

  // Free the previous table.
  for (size_t i = 0; i < _tableCapacity; ++i) {
    free(_table[i]);
  }
  free(_table);

  // Re-calculate the total capacity.
  _totalCapacity = kBlockSize * new_table_cap;
  // Reorder the hd and tl.
  _hd = new_hd;
  _tl = {0, 0};

  // Use the new table.
  _tableCapacity = new_table_cap;
  _table = new_table;
}

void CircularBuffer::decrease_table() {
  if (_tableCapacity == 1) return;
  // If usage rate is 1/4 -> decrease to 1/2.
  size_t new_table_cap = _tableCapacity >> 1;
  uint8_t** new_table = (uint8_t**)malloc(new_table_cap * sizeof(uint8_t*));
  for (size_t i = 0; i < new_table_cap; ++i) {
    new_table[i] = (uint8_t*)malloc(kBlockSize);
  }

  // Copy the previous table.
  size_t bytes_copied = 0;
  buf_pointer new_hd{0, 0};
  while (bytes_copied < _totalSize) {
    size_t in_block = kBlockSize - _tl.m;
    size_t bytes_to_copy = std::min(in_block, _totalSize - bytes_copied);

    memcpy(&new_table[new_hd.n][new_hd.m], &_table[_tl.n][_tl.m],
           bytes_to_copy);

    bytes_copied += bytes_to_copy;
    _tl = advance_buf_pointer(_tl, bytes_to_copy);
    new_hd = advance_buf_pointer(new_hd, bytes_to_copy);
  }

  // Free the previous table.
  for (int i = 0; i < _tableCapacity; ++i) {
    free(_table[i]);
  }
  free(_table);

  // Re-calculate the total capacity.
  _totalCapacity = kBlockSize * new_table_cap;
  // Reorder the hd and tl.
  _hd = new_hd;
  _tl = {0, 0};

  // Use the new table.
  _tableCapacity = new_table_cap;
  _table = new_table;
}

CircularBuffer::buf_pointer CircularBuffer::advance_buf_pointer(
    buf_pointer pointer, size_t size) const {
  size_t next_m = pointer.m + size;

  size_t d = next_m / kBlockSize;
  size_t r = next_m % kBlockSize;

  buf_pointer new_pointer{pointer};
  new_pointer.n = (new_pointer.n + d) % _tableCapacity;
  new_pointer.m = r;

  return new_pointer;
}

bool CircularBuffer::buf_pointer::operator==(const buf_pointer& other) {
  return n == other.n && m == other.m;
}

bool CircularBuffer::buf_pointer::operator!=(const buf_pointer& other) {
  return !(*this == other);
}

bool CircularBuffer::buf_pointer::operator<(const buf_pointer& other) {
  if (n == other.n) {
    return m < other.m;
  } else {
    return n < other.n;
  }
}
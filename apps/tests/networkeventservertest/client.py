#!/usr/bin/python
import asyncore, socket, struct

METADATAFORMAT = '<H'

class CS_DATATYPE:
  CS_DATATYPE_INT8 = 0, 'b'
  CS_DATATYPE_UINT8 = 1, 'B'
  CS_DATATYPE_INT16 = 2, 'h'
  CS_DATATYPE_UINT16 = 3, 'H'
  CS_DATATYPE_INT32 = 4, 'i'
  CS_DATATYPE_UINT32 = 5, 'I'
  CS_DATATYPE_INT64 = 6, 'q'
  CS_DATATYPE_UINT64 = 7, 'Q'
  CS_DATATYPE_DOUBLE = 8, 'd'
  CS_DATATYPE_DATABUFFER = 9, 'Q' #just the length
  CS_DATATYPE_EVENT = 10, 'Q' #just the length
  
  @classmethod
  def get(cls, int):
    for name in dir(cls):
      if name.startswith('CS_DATATYPE_') and getattr(cls, name)[0] == int:
        return name
  
  @classmethod
  def get_format(cls, int):
    for name in dir(cls):
      if name.startswith('CS_DATATYPE_') and getattr(cls, name)[0] == int:
        return getattr(cls, name)[1]

class Buffer():
  def __init__(self, buffer):
    self.buffer = buffer
    self.position = 0
  def Read(self, format):
    size = struct.calcsize(format)
    data = struct.unpack(format, self.buffer[self.position:self.position+size])
    if len(data) == 1:
      data = data[0]
    self.position += size
    return data
    
  def ReadString(self, size):
    data = self.buffer[self.position:self.position+size]
    self.position += size
    return data  

class Event():
  def __init__(self):
    self.name = None
    self.Time = None
    self.Broadcast = False
    self.attributes = {}
  def Add(self, name, value):
    self.attributes[name] = value
  def __str__(self):
    return self.name + ', '+ str(self.Time) +', ' + str(self.Broadcast) +', ' + str(self.attributes)

class csEventFlattener():
  @classmethod
  def UnFlatten(cls, event, buffer):
    buffer = Buffer(buffer)
    #Version(4) + packet length(8) + Time(4) + Broadcast(1) + Name Length(2) + Name(X)
    unpacked = buffer.Read('<IQIBH')
    name = buffer.ReadString(unpacked[4])
    event.name = name
    event.Time = unpacked[2]
    event.Broadcast = unpacked[3]

    size = unpacked[1]
    while buffer.position < size-1: #Where is this extra '\x00' comming from?
      ui16 = buffer.Read('<H')
      name = buffer.ReadString(ui16)
      
      ui8 = buffer.Read('<B')
      class switch:
        def handle(self, command, format):
          return getattr(self, command, getattr(self,'CS_DATATYPE_'))(command, format)
        def CS_DATATYPE_(self,command, format):
          return buffer.Read('<'+format)
        def CS_DATATYPE_DATABUFFER(self, command, format):
          size = buffer.Read('<'+format)
          return buffer.ReadString(size)
        def CS_DATATYPE_EVENT(self,command, format):
          size = buffer.Read('<'+format)
          data = buffer.ReadString(size)
          event = Event()
          cls.UnFlatten(event, data)
          return event
      value = switch().handle(CS_DATATYPE.get(ui8), CS_DATATYPE.get_format(ui8))
      event.Add(name, value)

class PendingPacket():
  def __init__(self):
    self.buffer = None
    self.processed = None
  @property
  def buffer_size(self):
    return len(self.buffer)

class CSClient(asyncore.dispatcher):
  def __init__(self):
    asyncore.dispatcher.__init__(self)
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.connect( ('localhost', 12345) )
    self.buffer = b''
    self.incoming = None

  def handle_connect(self):
    print('Connected!')

  def handle_close(self):
    print('Connection closed!')
    self.close()

  def handle_read(self):
    data = self.recv(0xFFF)
    processed = 0
    left = len(data)
    
    if self.incoming:
      missing = self.incoming.buffer_size - self.incoming.processed
      toProcess = min(missing, left)
      
      self.incoming.buffer[self.incoming.processed:self.incoming.processed+toProcess] = data[:toProcess]

      left -= toProcess
      processed += toProcess

      # check whether this request is done now
      if missing == toProcess:
        # create event
        self.CreateEvent(self.incoming.buffer, self.socket.getpeername());

        # we're done with the request
        self.incoming = None;
      else:
        # let the request know we did something
        self.incoming.processed += toProcess
    
    
    #process what's left
    while left > 0:
      # read meta-data
      size = struct.unpack_from(METADATAFORMAT, data[processed:])[0]
      #print('SIZE', size, dir(self))
      processed += struct.calcsize(METADATAFORMAT)
      left -= struct.calcsize(METADATAFORMAT)

      # check whether we have enough data left
      if size > left:
        # no, create a pending request
        self.incoming = PendingPacket()
        self.incoming.buffer = bytearray(size)

        # read what we can
        self.incoming.buffer[processed:processed+left] = data[:left]
        self.incoming.processed = left;
        left = 0;
      else:
        # create event
        self.CreateEvent(data[processed:processed+size], self.socket.getpeername());

        # update processed/left count
        processed += size
        left -= size


  def CreateEvent(self, buffer, address):
    event = Event()
    csEventFlattener.UnFlatten(event, buffer)
    print(event)
    
  def writable(self):
    return (len(self.buffer) > 0)

  def handle_write(self):
    sent = self.send(self.buffer)
    self.buffer = self.buffer[sent:]


client = CSClient()
asyncore.loop()



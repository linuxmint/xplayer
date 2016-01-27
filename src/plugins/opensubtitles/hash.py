import struct
import os
from gi.repository import Gio # pylint: disable-msg=E0611

SIZE_ERROR = -1
SEEK_ERROR = -2

def hash_file (name):
    """ FIXME Need to handle exceptions !! """

    longlongformat = 'q' # long long
    bytesize = struct.calcsize (longlongformat)

    file_to_hash = Gio.File.new_for_uri (name)

    file_info = file_to_hash.query_info ('standard::size', 0, None)
    filesize = file_info.get_attribute_uint64 ('standard::size')

    file_hash = filesize

    if filesize < 65536 * 2:
        return SIZE_ERROR, 0

    file_handle = file (file_to_hash.get_path (), "rb")

    for _ in range (65536 / bytesize):
        buf = file_handle.read (bytesize)
        (l_value,) = struct.unpack (longlongformat, buf)
        file_hash += l_value
        file_hash = file_hash & 0xFFFFFFFFFFFFFFFF #to remain as 64bit number

    file_handle.seek (max (0, filesize - 65536), os.SEEK_SET)

    if file_handle.tell() != max (0, filesize - 65536):
        return SEEK_ERROR, 0

    for _ in range (65536/bytesize):
        buf = file_handle.read (bytesize)
        (l_value,) = struct.unpack (longlongformat, buf)
        file_hash += l_value
        file_hash = file_hash & 0xFFFFFFFFFFFFFFFF

    file_handle.close ()
    returnedhash = "%016x" % file_hash

    return returnedhash, filesize


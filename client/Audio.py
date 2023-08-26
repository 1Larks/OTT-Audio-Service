import socket
import threading
import pyaudio
import sounddevice
import struct

CHUNK_SIZE=6144

class AudioHandler:
    def __init__(self, sock:socket.socket):
        self.sock=sock
        self.playing=False
        self.Audio = pyaudio.PyAudio()
        self.sync="COTNU"
        self.bytes_recieved=0
    
    def recieve_wav_header(self):
        header=self.sock.recv(44)

        # I got help on this function from chatgpt, 
        # it was super wierd so I shamley resortet to AI after not getting this shit to work

        # The format string for unpacking the WAV header
        # I found this on the internet, didn't think it was gonna work, but this voodoo magic thing has proven itself
        format_string = '<4sI4s4sIHHIIHH4sI'

        # Unpack the header data using the format string
        (riff_chunk_id, riff_chunk_size, format, subchunk1_id, subchunk1_size,
        audio_format, channels, sample_rate, byte_rate, block_align, bits_per_sample,
        subchunk2_id, subchunk2_size) = struct.unpack(format_string, header)

        # Print the extracted information
        return sample_rate, channels

    def start_audio_thread(self):
        thread=threading.Thread(target=self._playAudio)
        thread.start()

    def _playAudio(self):
        sample_rate, channels=self.recieve_wav_header()
        
        buffer_size=1024
        
        # Open an audio stream
        stream = self.Audio.open(
        format=pyaudio.paInt16,
        channels=channels,
        rate=sample_rate,
        output=True,
        frames_per_buffer=buffer_size)
        status=""
        while self.playing:
            #get a chunk of audio data
            chunk = self.sock.recv(CHUNK_SIZE)
            self.sock.send(self.sync.encode())
            if self.sync=="PAUSE":
                self.playing=False
                break
            if not chunk:
                self.bytes_recieved=0
                self.playing=False
                break
            #status=self.sock.recv(5).decode()
            
            #if status == "FNISH":
                #self.playing=False

            #play the chunk in the audio stream
            stream.write(chunk)
            self.bytes_recieved+=CHUNK_SIZE
        stream.stop_stream()
        stream.close()
# getting this thing to work was a challange, I was always very close to making it work, but the information on the
# internet was pretty useless and i was barely able to find any relevant information, but in the end I managed to build this
# simple and short solution, and it's working perfectly.


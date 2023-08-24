import socket
import threading
import pyaudio
import sounddevice

CHUNK_SIZE=6144

class AudioHandler:
    def __init__(self, sock:socket.socket):
        self.sock=sock
        self.playing=False
        self.Audio = pyaudio.PyAudio()
        self.sync="COTNU"
    
    def start_audio_thread(self):
        thread=threading.Thread(target=self._playAudio)
        thread.start()

    def _playAudio(self):
        sample_rate=44100
        channels=2
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
            print("Chunk recieved")
            self.sock.send(self.sync.encode())
            if self.sync=="PAUSE" or not chunk:
                self.playing=False
                break
            #status=self.sock.recv(5).decode()
            
            #if status == "FNISH":
                #self.playing=False

            #play the chunk in the audio stream
            stream.write(chunk)
        stream.stop_stream()
        stream.close()
# getting this thing to work was a challange, I was always very close to making it work, but the information on the
# internet was pretty useless and i was barely able to find any relevant information, but in the end I managed to build this
# simple and short solution, and it's working perfectly.


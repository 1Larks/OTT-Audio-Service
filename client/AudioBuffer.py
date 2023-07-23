import threading
import queue
import pyaudio
import time

ChunkSize=6144

class Audio_Buffer:
    def __init__(self):
        #(Chunk ID, Data)
        self.buffer=queue.PriorityQueue()
        self.playing=False
        self.audio=pyaudio.PyAudio()
    
    def addChunk(self, data, id):
        self.buffer.put((id, data))

    def createAudioStream(self):
        stream=self.audio.open(format=self.audio.get_format_from_width(2), channels=2, rate=44100, output=True)
        self.audioStream=stream

    def _playFromBuffer(self):
        while self.playing:
            try:
                _, data=self.buffer.get(timeout=1.0)
                self.audioStream.write(data)
            except:
                #buffer is empty, wait for more chunks (self.playing should be turned off
                #in the main function)
                continue
    def playAudio(self):
        self.playing=True
        self._playFromBuffer()
    
    def startAudioThread(self):
        playback_thread = threading.Thread(target=self.playAudio)
        playback_thread.start()
    
    def stopAudioThread(self):
        self.playing = False
        time.sleep(0.1)  # Allow some time for the playback thread to stop
        self.audio_stream.stop_stream()
        self.audio_stream.close()
        self.audio.terminate()
    
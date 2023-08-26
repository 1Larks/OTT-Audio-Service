import socket
import tkinter
import Audio



#TODO: login system, improve search system



# Server config
IP=""
PORT=31311
ADDR=(IP, PORT)
sock=socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
sock.connect(ADDR)



# Main window (root) config
root=tkinter.Tk()
root.title("Larks' OTT Service")
root.geometry('900x700')
root.resizable(False, False)
root['background']='#28282B'



# An array containing the all the played songs
last_songs=[]



# Audio handler init, from the Audio.py file
audio_handler=Audio.AudioHandler(sock)



# Creating an empty play button so it could appear when a song is played or paused
play_button=tkinter.Button



def stop_playing():
    audio_handler.sync="PAUSE"
    while audio_handler.playing:
        continue



def on_closing():
    if audio_handler.playing:
        stop_playing()

    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_closing)


# The function tells the audio_handler var to pause the current song and changes the play button

# A function to pause the currently played song
def pause_song():
    stop_playing()
    playButton=tkinter.Button(root, text="|>",command=play_song, font=("arial", 18), activebackground='black', 
                              activeforeground='#FAEDE3', background='#28282B', 
                                foreground='#FAEDE3',highlightbackground='#28282B', highlightcolor='#D9EDE3')
    playButton.pack()
    playButton.place(x=425, y=625, width=50, height=50)



# Changes the play button and starts the audio thread in the audio handler variable
def play_song(songID=""):
    playButton=tkinter.Button(root, text="||",command=pause_song,font=("arial", 18), activebackground='black', 
                              activeforeground='#FAEDE3', background='#28282B', 
                                foreground='#FAEDE3',highlightbackground='#28282B', highlightcolor='#D9EDE3')
    playButton.pack()
    playButton.place(x=425, y=625, width=50, height=50)
    if songID != "":
        last_songs.append(songID)
    sock.send(("PLAY:"+last_songs[len(last_songs)-1]).encode())
    audio_handler.sync="COTNU"
    audio_handler.playing=True
    audio_handler.start_audio_thread()



def play_new_song(songID=""):
    if audio_handler.playing:
        stop_playing()
        audio_handler.bytes_recieved=0
    play_song(songID)



# The search function sends the search request to the server like so: SRCH:entry, the server recieves the entry
# and starts searching in the songs file for matching songs, albums and artists, then if the server found any, it sends
# the results back to the client and then the client displays all the results

# A function to get and display search results from the server to the client
def search(entry: str):
    global sock, root

    msg="SRCH:"+entry
    sock.send(msg.encode())
    #search page, else=main page
    if entry!="":
        results=sock.recv(6)
        if results.decode()=="SRCSUC":
            results=sock.recv(2048)
            results=results.rstrip(b'\x00')
            results=results.decode()
            showingResults=tkinter.Label(root, anchor=tkinter.W, text=("Showing results for "+entry+':'),
                                         font=("arial", 22), background='#28282B', foreground='#FAEDE3')
            showingResults.pack()
            showingResults.place(x=10, y=40, width=900, height=75)
            if(results==""): #if no results found
                showingResults.configure(text="No results found for " + entry)
            else:
                allFound=results.split('\n')
                print(results)
                songLabel=tkinter.Label(root, font=("arial", 19), anchor=tkinter.W,text="Songs:", 
                                        background='#28282B', foreground='#FAEDE3')
                songLabel.pack()
                songLabel.place(x=10, y=120, width=900, height=75)
                albumLabel=tkinter.Label(root,font=("arial", 19), anchor=tkinter.W,text="Albums:", 
                                         background='#28282B', foreground='#FAEDE3')
                albumLabel.pack()
                albumLabel.place(x=10, y=295, width=900, height=75)
                artistLabel=tkinter.Label(root, font=("arial", 19), anchor=tkinter.W,text="Artists:", 
                                          background='#28282B', foreground='#FAEDE3')
                artistLabel.pack()
                artistLabel.place(x=10, y=470, width=900, height=75)
                count=[0,0,0]
                prevWidth=[0,0,0]
                for i in range(len(allFound)-1):
                    splitted=allFound[i].split(';')
                    if str.lower(entry) in str.lower(splitted[0]):
                        name=splitted[0]
                        album=splitted[1]
                        artist=splitted[2]
                        count[0]+=1
                        width=(max([len(name), len(album), len(artist)])*10)+30
                        song=tkinter.Button(root, anchor=tkinter.W,
                                            command=lambda id=splitted[3]: play_new_song(id), background='#28282B',
                                             foreground='#FAEDE3', activebackground='black', activeforeground='#FAEDE3')
                        song.pack()
                        song.place(x=(10+ (i*(prevWidth[0]+20))), y=200, width=width, height=75)
                        song.configure(text=(name+'\nBy: '+artist+"\nIn: "+album))
                        prevWidth[0]=width

                    if str.lower(entry) in str.lower(splitted[1]):
                        album=splitted[1]
                        artist=splitted[2]
                        count[1]+=1
                        width=(max([len(album), len(artist)])*10)+30
                        
                        song=tkinter.Button(root, anchor=tkinter.W, background='#28282B', foreground='#FAEDE3', 
                                            activebackground='black', activeforeground='#FAEDE3')
                        song.pack()
                        song.place(x=(10+ (i*(prevWidth[1]+20))), y=375, width=width, height=75)
                        song.configure(text=(album+'\nBy: '+artist))
                        prevWidth[1]=width
                    if str.lower(entry) in str.lower(splitted[2]):
                        artist=splitted[2]
                        count[2]+=1
                        width=(len(artist)*10)+30
                        
                        song=tkinter.Button(root, anchor=tkinter.W, background='#28282B', foreground='#FAEDE3', 
                                            activebackground='black', activeforeground='#FAEDE3')
                        song.pack()
                        song.place(x=(10+ (i*(prevWidth[2]+20))), y=550, width=width, height=75)
                        song.configure(text=artist)
                        prevWidth[2]=width

                if count[0]==0:
                    noSongs=tkinter.Label(root, anchor=tkinter.W,font=("arial", 19),text="No songs found",
                                          background='#28282B', foreground='#FAEDE3')
                    noSongs.pack()
                    noSongs.place(x=40, y=200, width=250, height=30)
                if count[1]==0:
                    noSongs=tkinter.Label(root, anchor=tkinter.W,font=("arial", 19),text="No albums found",
                                          background='#28282B', foreground='#FAEDE3')
                    noSongs.pack()
                    noSongs.place(x=40, y=375, width=250, height=30)
                if count[2]==0:
                    noSongs=tkinter.Label(root, anchor=tkinter.W,font=("arial", 19),text="No artists found",
                                          background='#28282B', foreground='#FAEDE3')
                    noSongs.pack()
                    noSongs.place(x=40, y=550, width=250, height=30)
        else:
            errorLabel=tkinter.Label(root, anchor=tkinter.W, text="Error getting search results from the server.",
                                         font=("arial", 22), background='#28282B', foreground='#FAEDE3')



#TEMPORARY
def login(username:str, password:str):
    global sock
    info=username+':'+password
    sock.send(info.encode())
    recieved=sock.recv(6)

    if recieved.decode()=="LOGERR":
        pass
    else:
        global USERNAME
        USERNAME=username
        main_page()



# Login page
def login_page():
    global root, sock
    clear_window()
    userField=tkinter.Entry(root, font=("arial", 18), background='#28282B', foreground='#FAEDE3', highlightbackground='#28282B', highlightcolor='#D9EDE3')
    userField.pack()
    userField.place(x=300, y=200, width=300, height=50)

    userLabel=tkinter.Label(root, text="Username", font=("arial", 18), background='#28282B', foreground='#FAEDE3')
    userLabel.pack()
    userLabel.place(x=300, y=150, width=300, height=50)

    passField=tkinter.Entry(root, show="*",font=("arial", 18), background='#28282B', foreground='#FAEDE3', highlightbackground='#28282B', highlightcolor='#D9EDE3')
    passField.pack()
    passField.place(x=300, y=300, width=300, height=50)

    passLabel=tkinter.Label(root,text="Password", font=("arial", 18), background='#28282B', foreground='#FAEDE3')
    passLabel.pack()
    passLabel.place(x=300, y=250, width=300, height=50)
    
    def validLogin():
        username=userField.get()
        password=passField.get()
        error=False
        Text=""
        Font=("arial",14)
        if username=="" and password=="":
            Text="You have to enter a username and a password."
            Font=("arial", 10)
            error=True
        elif username=="":
            Text="You have to enter a username."
            error=True
        elif password=="":
            Text="You have to enter a password."
            error=True
        elif " " in password:
            Text="Your password cannot contain spaces."
            Font=("arial", 12)
            error=True
        elif len(password)>=15 or len(username)>=15:
            Text="Both username and password's length should be 15 or lower."
            Font=("arial", 10)
            error=True
        elif username[0]==" " or password[0]==" ":
            Text="Your username and password cannot have leading spaces."
            Font=("arial", 10)
            error=True
        elif ":" in username:
            Text="Your username and password cannot have leading spaces."
            Font=("arial", 10)
            error=True
        else:
            login(username, password)
        
        if error:
            errLabel=tkinter.Label(root, text=Text, font=Font, background='#28282B', foreground='#D01818')
            errLabel.pack()
            errLabel.place(x=300, y=425, width=300, height=50)
        

    loginButton=tkinter.Button(root, text='Login',font=("arial", 18), command=validLogin, activebackground='black', activeforeground='#FAEDE3', background='#28282B', foreground='#FAEDE3',highlightbackground='#28282B', highlightcolor='#D9EDE3')
    loginButton.pack()
    loginButton.place(x=347, y=375, width=200, height=50)
    
    root.mainloop()
    sock.close()


# Main page
def main_page(entry: str=""):
    global root, sock
    clear_window()
    SearchBar=tkinter.Entry(root,width=15, font=("arial", 18), background='#28282B', foreground='#FAEDE3')
    SearchBar.insert(0, entry)
    SearchBar.pack()
    SearchBar.place(x=250, y=5, width=300, height=30)
    
    def validSearch():
        Entry=SearchBar.get()
        if len(Entry)>0 and Entry[0]!=" ":
            search(Entry)
        else:
            pass
            
    SearchButton=tkinter.Button(text='Search', command=validSearch, 
                                activebackground='black', activeforeground='#FAEDE3', background='#28282B', 
                                foreground='#FAEDE3',highlightbackground='#28282B', highlightcolor='#D9EDE3')
    SearchButton.pack()
    SearchButton.place(x=551, y=5, width=50, height=30)
    
    WelcomeLabel=tkinter.Label(root, text=("Hello, "+USERNAME), font=("arial", 14), background='#28282B', foreground='#FAEDE3')
    WelcomeLabel.pack()
    WelcomeLabel.place(x=600, y=10, width=250, height=25)

    

    root.mainloop()
    sock.close()



def main():
    login_page()



# Clears the main window of all its widgets
def clear_window():
    global root
    for widget in root.winfo_children():
        widget.destroy()




if __name__=='__main__':
    main()

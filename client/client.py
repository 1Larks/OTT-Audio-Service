import socket
import tkinter

ADDR=("10.0.2.15", 31311)

#username
USERNAME=""

#general window config
root=tkinter.Tk()
root.title("Larks' OTT Service!")
root.geometry('900x700')
root.resizable(False, False)
root['background']='#28282B'

sock=socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
sock.connect(ADDR)

def search(entry: str):
    global sock

    msg="SEARCH"+entry
    sock.send(msg.encode())
    mainPage(entry)

#TEMPORARY: CHANGE ASAP!!!!!
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
        mainPage()

def loginPage():
    global root, sock
    clearWindow()
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
        

    loginButton=tkinter.Button(root, text='Login',font=("arial", 18), command=validLogin, activebackground='#28282B', activeforeground='#FAEDE3', background='#28282B', foreground='#FAEDE3',highlightbackground='#28282B', highlightcolor='#D9EDE3')
    loginButton.pack()
    loginButton.place(x=347, y=375, width=200, height=50)
    
    root.mainloop()
    sock.close()
    

def mainPage(entry: str=""):
    global root, sock
    clearWindow()
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
            
    SearchButton=tkinter.Button(text='Search', command=validSearch, activebackground='#28282B', activeforeground='#FAEDE3', background='#28282B', foreground='#FAEDE3',highlightbackground='#28282B', highlightcolor='#D9EDE3')
    SearchButton.pack()
    SearchButton.place(x=551, y=5, width=50, height=30)
    
    WelcomeLabel=tkinter.Label(root, text=USERNAME, font=("arial", 14), background='#28282B', foreground='#FAEDE3')
    WelcomeLabel.pack()
    WelcomeLabel.place(x=600, y=10, width=250, height=25)

    #search page, else=main page
    if entry!="":
        results=sock.recv(6)
        if results.decode()=="SRCSUC":
            results=sock.recv(2048)
            results=results.rstrip(b'\x00')
            results=results.decode()
            showingResults=tkinter.Label(root, text=("Showing results for "+entry+':'),
                                         font=("arial", 22), background='#28282B', foreground='#FAEDE3').grid(row=0, column=0, sticky=tkinter.W)
            showingResults.pack()
            showingResults.place(x=10, y=40, width=900, height=75)
            if(results==""): #if no results found
                showingResults.configure(text="No results found for " + entry)
            else:
                allFound=results.split('\n')
                songLabel=tkinter.Label(root, anchor=tkinter.W,text="Songs:", background='#28282B', foreground='#FAEDE3').grid(row=1, column=0, sticky=tkinter.W, pady=2)
                songLabel.pack()
                albumLabel=tkinter.Label(root, anchor=tkinter.W,text="Albums:", background='#28282B', foreground='#FAEDE3').grid(row=2, column=0, sticky=tkinter.W, pady=2)
                albumLabel.pack()
                artistLabel=tkinter.Label(root, anchor=tkinter.W,text="Artists:", background='#28282B', foreground='#FAEDE3').grid(row=3, column=0, sticky=tkinter.W, pady=2)
                artistLabel.pack()
                countSongs=0
                countAlbums=0
                countArtists=0
                for i in range(len(allFound)-1):
                    splitted=allFound[i].split('$')
                    if entry in splitted[0]:
                        name=splitted[0]
                        album=splitted[1]
                        artist=splitted[2]
                        countSongs+=1
                        
                    if entry in splitted[1]:
                        album=splitted[1]
                        artist=splitted[2]
                        countAlbums+=1
                    if entry in splitted[2]:
                        artist=splitted[2]
                        countArtists+=1
        else:
            pass

    root.mainloop()
    sock.close()

def main():
    loginPage()

def clearWindow():
    global root
    for widget in root.winfo_children():
        widget.destroy()

if __name__=='__main__':
    main()

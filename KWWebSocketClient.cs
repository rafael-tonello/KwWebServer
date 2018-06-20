using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace KW
{
    public class KWWebSocketClient
    {
        /// <summary>
        /// Delegate used to dispatch the data received from websocket connection. Both parameters contaisn the same data
        /// </summary>
        /// <param name="dataStr">The data received as string</param>
        /// <param name="dataArray">Thre raw data received (Buffer)</param>
        public delegate void __OnData(string dataStr, byte[] dataArray);

        /// <summary>
        /// Event dispatched when the connection is stablished
        /// </summary>
        public Action OnConnect;

        /// <summary>
        /// Event dipatched when connection is closed
        /// </summary>
        public Action OnDisconnect;

        /// <summary>
        /// Event triggered when data is received from connection
        /// </summary>
        public __OnData OnData;



        private ClientWebSocket webSocket;

        /// <summary>
        /// Send an string to server
        /// </summary>
        /// <param name="toSend"></param>
        public bool SendString(string toSend)
        {
            return this.SendBuffer(Encoding.UTF8.GetBytes(toSend));
        }


        //Send an buffer to server
        public bool SendBuffer(byte[] buffer)
        {
            if ((webSocket != null) && (webSocket.State == WebSocketState.Open))
            {
                ArraySegment<byte> send = new ArraySegment<byte>(buffer);
                try
                {
                    webSocket.SendAsync(send, WebSocketMessageType.Binary, true, CancellationToken.None);
                    return true;
                }
                catch { }
            }

            return false;
        }

        //try to open connection to the server
        public bool Open(string server)
        {
            if (webSocket == null || webSocket.State == WebSocketState.Aborted || webSocket.State == WebSocketState.Closed || webSocket.State == WebSocketState.None)
            {
                webSocket = new ClientWebSocket();
                webSocket.ConnectAsync(new Uri(server), CancellationToken.None);

                Thread th = new Thread(delegate ()
                {
                    ReadThread();
                });
                th.Start();


                Thread.Sleep(100);
                while (webSocket.State == WebSocketState.Connecting)
                {
                    Thread.Sleep(1);
                }

                return webSocket.State == WebSocketState.Open;
            }
            else
                return false;
        }

        //Closes the connection
        public void Close()
        {

        }

        private void ReadThread()
        {
            while (webSocket.State == WebSocketState.Connecting)
            {
                Thread.Sleep(1);
            }

            var buf = new byte[4096];
            ArraySegment<byte> dataRec = new ArraySegment<byte>(buf);
            Task.Run(async () =>
            {
                if (this.OnConnect != null)
                    this.OnConnect();

                while (webSocket.State == WebSocketState.Open)
                {
                    var a = await webSocket.ReceiveAsync(dataRec, CancellationToken.None);
                    if (dataRec.Count > 0)
                    {
                        byte[] buf2 = buf.Take(a.Count).ToArray();
                        string dataStr = Encoding.UTF8.GetString(buf2);
                        if (this.OnData != null)
                            this.OnData(dataStr, buf2);
                    }
                }

                try
                {
                    webSocket.Dispose();
                }
                catch { }

                if (this.OnDisconnect != null)
                    this.OnDisconnect();
            });

        }

        public enum Status { Disconnected, Connecting, Connected };
        public Status getStatus()
        {
            if (webSocket != null)
            {
                if (webSocket.State == WebSocketState.Connecting)
                    return Status.Connecting;
                else if (webSocket.State == WebSocketState.Open)
                    return Status.Connected;                       
            }

            return Status.Disconnected;
        }
    }
}

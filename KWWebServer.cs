/*
    Kiwiisco embeded webserver
    By: Refael Tonello (tonello.rafinha@gmail.com)
 Versão 1.1  
 
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace Libs.IO
{

    public class MHttpServer
    {
        public enum Method { POST, GET, UNKNOWN };
        public delegate string onDataEvent(Method method, string getUrl, string body, out byte[] opBinary, out string opMime, out int opHttpStatus, Socket tcpClient);

        public event onDataEvent onClienteDataSend;


        private TcpListener myListener;
        private int port = 5050;  // Select any free port you wish 
        private bool autoSendFiles;
        private bool rodando;
        private Thread th;
        private string filesFolder = "";

        public delegate byte[] OnReadyToSendDelegate(Method method, string getUrl, string body, string mimeType, byte[] buffer, Socket tcpClient);

        //retornar null para evitar processamento excessivo (executado antes de receber o cabeçalho)
        public OnReadyToSendDelegate onContentReady;

        //retornar null para evitar processamento excessivo (executado antes de enviar o conteúdo para o browser)
        public OnReadyToSendDelegate onReadyToSend;

        


        public MHttpServer(int port, bool pautoSendFiles, string autoSendFiles_fodler = "")
        {
            this.autoSendFiles = pautoSendFiles;
            this.filesFolder = autoSendFiles_fodler;
            if ((this.filesFolder.Length > 0) && (this.filesFolder[this.filesFolder.Length-1] != '\\'))
                this.filesFolder += '\\';

            try
            {
                //start listing on the given port
                myListener = new TcpListener(port);
                myListener.Start();

                //start the thread which calls the method 'StartListen'
                th = new Thread(new ThreadStart(StartListen));
                th.Start();

            }
            catch (Exception e)
            {
            }

        }

        ~MHttpServer()
        {
            this.StopWork();
        }

        public void loadFile(string getUrl, out byte[] binaryResp, out string mime, out int httpStatus)
        {
            //EVITA A SAída da pasta, para proteger o servidor
            //substitui em um while para evitar "montagens" com a uri
            //utiliza um sistema máxim de contagem de loops para evitar travamentos
            /*int loops = 0;
            while (getUrl.IndexOf("..") > -1)
            {
                getUrl = getUrl.Replace("..", "");
                loops++;
                if (loops >= 10)
                    return;
            }*/

            if (getUrl.IndexOf("..") > -1)
            {
                binaryResp = new byte[0];
                mime = "";
                httpStatus = 0;
                return;
            }

            mime = "text/html";
            if (getUrl.IndexOf('?') > -1)
                getUrl = getUrl.Split('?')[0];
            try
            {
                httpStatus = 200;
                if (getUrl == "/")
                {
                    mime = "text/html";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + "index.htm");
                }
                else if (getUrl.IndexOf(".gif") > -1)
                {
                    mime = "image/gif";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".jpg") > -1)
                {
                    mime = "image/jpeg";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".png") > -1)
                {
                    mime = "image/png";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".js") > -1)
                {
                    mime = "text/javascript";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".css") > -1)
                {
                    mime = "text/css";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + getUrl.Split('=')[1]);
                }
                else
                {
                    mime = "text/html";
                    binaryResp = System.IO.File.ReadAllBytes(filesFolder + getUrl.Split('=')[1]);
                }
            }
            catch (Exception e)
            {
                int cont;
                binaryResp = new byte[e.Message.Length];
                for (cont = 0; cont < e.Message.Length; cont++)
                    binaryResp[cont] = Convert.ToByte(e.Message[cont]);
                httpStatus = 500;
            }
        }


        public void StopWork()
        {
            if (myListener != null)
                myListener.Stop();
            rodando = false;
            myListener = null;
            th.Interrupt();
        }

        private void StartListen()
        {
            rodando = true;
            byte[] buffer;
            string dataSend = "";
            string getUrl;
            string msg = "";
            byte[] binaryResp = null;
            string mime = "";
            int cont;
            int status = 200;

            byte[] tempBuf;
            Socket mySocket = null;
            string sBuffer = "";
            int contentLength = 0;
            string tempS;

            int contentStart = 0;
            //make a byte array and receive data from the client 
            Byte[] bReceive;// = new Byte[1024];
            string body;
            Method method;

            while (rodando)
            {
                //Accept a new connection
                if (myListener == null)
                    break;

                if (myListener.Pending())
                    mySocket = myListener.AcceptSocket();
                else
                {
                    try
                    {
                        Thread.Sleep(10);
                        continue;
                    }
                    catch (Exception e) { continue; }
                }



                mySocket.ReceiveTimeout = 2500;

                if (mySocket.Connected)
                {

                    
                    try
                    {
                        sBuffer = "";
                        int i = 1;
                        while ((i > 0) && (mySocket.Available > 0))
                        {
                            try
                            {

                                bReceive = new byte[mySocket.Available];

                                i = mySocket.Receive(bReceive, mySocket.Available, 0);
                                sBuffer += Encoding.ASCII.GetString(bReceive);
                            }
                            catch { break; }
                        }
                    }
                    catch (Exception e)
                    {
                        mySocket.Close();
                        continue;
                    }

                    
                    try
                    {
                        contentStart = 0;
                        //Convert Byte to String
                        

                        //verifica se já recebeu todo o conteúdo
                        if ((sBuffer.IndexOf("\r\n\r\n") > -1) && (sBuffer.IndexOf("\n\n") > -1) && (sBuffer.IndexOf("\r\r") > -1))
                            continue;

                        //verifica se possui o "content-length"
                        if (sBuffer.ToLower().IndexOf("content-length:") > -1)
                        {
                            //caso tenha recebido o content-length, verififca se recebeu todo o conteúdo
                            tempS = sBuffer.Substring(sBuffer.ToLower().IndexOf("content-length:"));
                            tempS = tempS.ToLower();
                            tempS = tempS.Split('\n')[0];
                            tempS = tempS.Replace("content-length:", "");
                            tempS = tempS.Replace(" ", "");
                            tempS = tempS.Replace("\r", "");
                            contentLength = Int32.Parse(tempS);
                            
                            contentStart = sBuffer.IndexOf("\r\n\r\n") + 4;
                            if (contentStart == 3)
                                contentStart = sBuffer.IndexOf("\n\n") + 2;
                            if (contentStart == 1)
                                contentStart = sBuffer.IndexOf("\r\r") + 2;
                            
                            if ((sBuffer.Length - contentStart) < contentLength)
                                continue;
                        }

                        //pega a url requisitada
                    
                        getUrl = sBuffer.Substring(sBuffer.IndexOf(' ')+1);// + 1, sBuffer.IndexOf('\n') - sBuffer.ToUpper().IndexOf(" HTTP"));
                        getUrl = getUrl.Substring(0, getUrl.ToUpper().IndexOf(" HTTP"));
                        body = "";
                        if (contentStart > 0)
                            body = sBuffer.Substring(contentStart);
                        
                        method =  Method.UNKNOWN;
                        if (sBuffer.Substring(0, 3) == "GET")
                            method = Method.GET;
                        else if (sBuffer.Substring(0, 4) == "POST")
                            method = Method.POST;
                    }
                    catch { continue; }
                    if (getUrl == "")
                        continue;

                    

                    //getUrl = System.Text.RegularExpressions.Regex.Unescape(getUrl);
                    //pega os dados que são enviados por post
                    
                    //verifica se já possui todo o conteudo


                    //At present we will only deal with GET type
                    if (method != Method.UNKNOWN)
                    {

                        status = 200;

                        msg = null;
                        binaryResp = null;
                        if (this.onClienteDataSend != null)
                        {
                            msg = this.onClienteDataSend(method, getUrl, body, out binaryResp, out mime, out status, mySocket);
                        }

                        if ((autoSendFiles) && ((getUrl.ToLower().IndexOf("file=") > -1) || (getUrl == "/") || (getUrl.ToLower() == "/favicon.ico") || (getUrl.ToLower() == "/index.htm") || (getUrl.ToLower() == "/index.html")) && (binaryResp == null) && (msg == null))
                        {
                            loadFile(getUrl, out binaryResp, out mime, out status);
                        }



                        if (binaryResp == null)
                        {
                            if (msg == null)
                                msg = "";

                            if (onContentReady != null)
                            {
                                byte[] temp = onContentReady(method, getUrl, body, "text/html", System.Text.Encoding.Default.GetBytes(msg), mySocket);
                                if (temp != null)
                                    msg = System.Text.Encoding.Default.GetString(temp);
                            }

                            dataSend = "HTTP/1.1 200 OK\n" +
                                //"Date: Mon, 23 May 2005 22:38:34 GMT\n" +
                                                                //"Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\n" +
                                                                "Server: Kiwiisco Webserver 1.1, embedded version\n" +
                                //"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n" +
                                //"ETag: \"3f80f-1b6-3e1cb03b\"\n" +
                                                                "Content-Type: text/html; charset=UTF-8\n" +
                                                                "Content-Length: " + Convert.ToString(msg.Length) + "\n" +
                                                                "Accept-Ranges: bytes\n" +
                                                                "Connection: close\n\n" + msg;

                            mime = "text/html";
                            buffer = Encoding.ASCII.GetBytes(dataSend);
                        }
                        else
                        {
                            if (onContentReady != null)
                            {
                                byte[] temp = onContentReady(method, getUrl, body, mime, binaryResp, mySocket);
                                if (temp != null)
                                    binaryResp = temp;
                            }

                            /*status = 401;
                            "WWW-Authenticate:	Basic realm=\"TP-LINK Wireless N Router WR841N\"" +*/

                            dataSend = "HTTP/1.1 " + status.ToString() + " OK\n" +  
                                    
                                //"Date: Mon, 23 May 2005 22:38:34 GMT\n" +
                                                                //"Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\n" +
                                                                "Server: Kiwiisco Webserver 1.1, embedded version\n" +
                                //"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n" +
                                //"ETag: \"3f80f-1b6-3e1cb03b\"\n" +
                                                                "Content-Type: " + mime + "\n" +
                                                                "Content-Length: " + Convert.ToString(binaryResp.Length) + "\n" +
                                                                "Accept-Ranges: bytes\n" +
                                                                "Connection: close\n\n";
                            

                            //coloca os dados binários no buffer
                            buffer = new byte[dataSend.Length + binaryResp.Length];
                            for (cont = 0; cont < dataSend.Length; cont++)
                                buffer[cont] = Convert.ToByte(dataSend[cont]);

                            for (cont = 0; cont < binaryResp.Length; cont++)
                                buffer[cont + dataSend.Length] = binaryResp[cont];



                        }

                        
                        if (onReadyToSend != null)
                        {
                            byte[] temp = onReadyToSend(method, getUrl, body, mime, buffer, mySocket);
                            if (temp != null)
                                buffer = temp;
                        }

                        mySocket.Send(buffer);
                        mySocket.Close();
                    }
                }
            }
        }
    }
}

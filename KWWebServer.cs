/*
    Kiwiisco embeded webserver
    By: Refael Tonello (tonello.rafinha@gmail.com)
    Versão 2.0, 15/06/2016
  
   changeLog:
       -added https support
 
  
  Http example
           KW.KWHttpServer temp = new KW.KWHttpServer(80, false);
            temp.onClienteDataSend += delegate(string method, string getUrl, string headerStr, string body, out byte[] opBinary, out string opMime, out int opHttpStatus, out string opHeaders, System.Net.Sockets.Socket tcpClient)
            {
                opBinary = null;
                opMime = "";
                opHttpStatus = 200;
                opHeaders = "";


                return "lol, sem https";
            };
  
  Https example
           KW.KWHttpServer temp = new KW.KWHttpServer(443, false);
            temp.enableHttps(@"C:\Users\engenharia4\Desktop\https\dinv001.pfx", "inovadaq");
            temp.onClienteDataSend += delegate(string method, string getUrl, string headerStr, string body, out byte[] opBinary, out string opMime, out int opHttpStatus, out string opHeaders, System.Net.Sockets.Socket tcpClient)
            {
                opBinary = null;
                opMime = "";
                opHttpStatus = 200;
                opHeaders = "";


                return "lol, com https";
            };
 
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.IO;

namespace KW
{

    public class KWHttpServer
    {
        X509Certificate serverCertificate = null;
        bool useHttps = false;
        public class Methods
        {

            public static string POST { get { return "POST"; } }
            public static string GET { get { return "GET"; } }
            public static string PUT { get { return "PUT"; } }
            public static string DELETE { get { return "DELETE"; } }
            public static string UNKNOWN { get { return "UNKNOWN"; } }


        };
        public delegate string onDataEvent(String method, string getUrl, string headerStr, string body, out byte[] opBinary, out string opMime, out int opHttpStatus, out string opHeaders, Socket tcpClient);

        public event onDataEvent onClienteDataSend;


        private TcpListener myListener;
        private int port = 5050;  // Select any free port you wish 
        private bool conf_autoSendFiles;
        private string conf_filesFolder = "";
        private bool conf_multiThread = true;
        private bool conf_keepAlive = false;
        private bool rodando;
        private Thread th;

        public delegate byte[] OnReadyToSendDelegate(string method, string getUrl, string body, string mimeType, byte[] buffer, Socket tcpClient);

        //retornar null para evitar processamento excessivo (executado antes de receber o cabeçalho)
        public OnReadyToSendDelegate onContentReady;

        //retornar null para evitar processamento excessivo (executado antes de enviar o conteúdo para o browser)
        public OnReadyToSendDelegate onReadyToSend;




        public KWHttpServer(int port, bool pconf_autoSendFiles, string conf_autoSendFiles_fodler = "", bool use_multiThreads = true, bool useKeepAliveConnection = false)
        {
            this.conf_multiThread = use_multiThreads;
            this.conf_autoSendFiles = pconf_autoSendFiles;
            this.conf_filesFolder = conf_autoSendFiles_fodler;
            this.conf_keepAlive = useKeepAliveConnection;

            if ((this.conf_filesFolder.Length > 0) && (this.conf_filesFolder[this.conf_filesFolder.Length - 1] != '\\'))
                this.conf_filesFolder += '\\';

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

        public bool enableHttps(string pfxCert, string pfxPass)
        {
            try
            {
                serverCertificate = new X509Certificate(pfxCert, pfxPass);//@"C:\Users\engenharia4\Desktop\https\dinv001.pfx", "inovadaq");//X509Certificate2.CreateFromCertFile(@"C:\Users\engenharia4\Desktop\https\server.pfx", "inovadaq");
                useHttps = true;
            }
            catch { useHttps = false; }

            return useHttps;
        }

        ~KWHttpServer()
        {
            this.StopWork();
        }

        public void loadFile(string getUrl, out byte[] binaryResp, out string mime, out int httpStatus)
        {
            //EVITA A SAída da pasta, para proteger o servidor
            //substitui em um while para evitar "montagens" com a uri
            //utiliza um sistema máxim de contagem de loops para evitar travamentos
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
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + "index.htm");
                }
                else if (getUrl.IndexOf(".gif") > -1)
                {
                    mime = "image/gif";
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".jpg") > -1)
                {
                    mime = "image/jpeg";
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".png") > -1)
                {
                    mime = "image/png";
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".js") > -1)
                {
                    mime = "text/javascript";
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + getUrl.Split('=')[1]);
                }
                else if (getUrl.IndexOf(".css") > -1)
                {
                    mime = "text/css";
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + getUrl.Split('=')[1]);
                }
                else
                {
                    mime = "text/html";
                    binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + getUrl.Split('=')[1]);
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

        List<Thread> threads = new List<Thread>();
        private void StartListen()
        {
            rodando = true;
            TcpClient mySocket = null;

            while (rodando)
            {
                //Accept a new connection
                if (myListener == null)
                    break;

                if (myListener.Pending())
                {
                    mySocket = myListener.AcceptTcpClient();
                    if (this.conf_multiThread)
                    {
                        //cria uma thread para escutar o socket
                        int index = threads.Count;


                        Thread temp = new Thread(delegate() { escutaCliente(mySocket, index); });
                        threads.Add(temp);
                        temp.Start();
                    }
                    else
                        this.escutaCliente(mySocket, -1);
                }
                else
                {
                    try
                    {
                        Thread.Sleep(10);
                        continue;
                    }
                    catch (Exception e) { continue; }
                }
            }
            return;
        }

        /*private void log(string msg, bool notifyController = true)
        {
            msg = DateTime.Now.ToString() + ": " + msg;
            try
            {
                string appPath = System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath) + "\\";
                if (System.IO.File.Exists(appPath + "kwwebserver.log"))
                    System.IO.File.AppendAllText(appPath + "kwwebserver.log", msg + "\n");
                else
                    System.IO.File.WriteAllText(appPath + "kwwebserver.log", msg + "\n");
            }
            catch { }
        }*/



        //%%%%%% Converter para máquina de estados (recebndoCabeçalho, recebendoConteúdo, processandoDados, RetornandoDados, FinalziandoConexao)
        public void escutaCliente(TcpClient mySocket, int threadIndex)
        {
            byte[] buffer;
            string dataSend = "";
            string getUrl = "";
            string msg = "";
            byte[] binaryResp = null;
            string mime = "";
            int cont;
            int status = 200;
            string opHeaders = "";
            byte[] tempBuf;
            string sBuffer = "";
            int contentLength = 0;
            string tempS;
            int contentStart = 0;
            Byte[] bReceive;// = new Byte[1024];
            string body = "";
            string method = Methods.UNKNOWN;

            string strHeader = "";

            int readTimeout = 10000;
            mySocket.ReceiveTimeout = 2500;

            Stream sslStream = null;
            if (useHttps)
                sslStream = new SslStream(mySocket.GetStream(), false);
            else
                sslStream = new NetworkStream(mySocket.Client);
            try
            {
                if (useHttps)
                    ((SslStream)sslStream).AuthenticateAsServer(serverCertificate, false, SslProtocols.Tls, true);

                while ((mySocket != null) && (mySocket.Connected))
                {
                    readTimeout -= 10;

                    if (readTimeout < 0)
                    {
                        mySocket.Close();
                        return;
                    }

                    try
                    {

                        sslStream.ReadTimeout = 1000;
                        sslStream.WriteTimeout = 1000;
                        
                        sBuffer = ReadMessage(sslStream);
                    }
                    catch (Exception e)
                    {
                        mySocket.Close();
                        break;
                    }

                    try
                    {
                        contentStart = 0;

                        //verifica se já recebeu todo o cabeçalho
                        if (sBuffer.IndexOf("\r\n\r\n") > -1)
                            strHeader = sBuffer.Substring(0, sBuffer.IndexOf("\r\n\r\n"));
                        else if (sBuffer.IndexOf("\n\n") > -1)
                            strHeader = sBuffer.Substring(0, sBuffer.IndexOf("\n\n"));
                        else if (sBuffer.IndexOf("\r\r") > -1)
                            strHeader = sBuffer.Substring(0, sBuffer.IndexOf("\r\r"));
                        else
                        {

                            Thread.Sleep(10);

                            continue;
                        }


                        if (strHeader != "")
                        {
                            contentStart = sBuffer.IndexOf("\r\n\r\n") + 4;
                            if (contentStart == 3)
                                contentStart = sBuffer.IndexOf("\n\n") + 2;
                            if (contentStart == 1)
                                contentStart = sBuffer.IndexOf("\r\r") + 2;

                            //verifica se possui o "content-length"
                            if (strHeader.ToLower().IndexOf("content-length:") > -1)
                            {
                                //caso tenha recebido o content-length, verififca se recebeu todo o conteúdo
                                tempS = strHeader.Substring(sBuffer.ToLower().IndexOf("content-length:"));
                                tempS = tempS.ToLower();
                                tempS = tempS.Split('\n')[0];
                                tempS = tempS.Replace("content-length:", "");
                                tempS = tempS.Replace(" ", "");
                                tempS = tempS.Replace("\r", "");
                                contentLength = Int32.Parse(tempS);




                                if ((sBuffer.Length - contentStart) < contentLength)
                                {

                                    Thread.Sleep(10);

                                    continue;
                                }
                            }

                            //pega a url requisitada
                            getUrl = sBuffer.Substring(sBuffer.IndexOf(' ') + 1);// + 1, sBuffer.IndexOf('\n') - sBuffer.ToUpper().IndexOf(" HTTP"));
                            getUrl = getUrl.Substring(0, getUrl.ToUpper().IndexOf(" HTTP"));
                            body = "";

                            if (contentStart > 0)
                                body = sBuffer.Substring(contentStart);

                            method = sBuffer.Substring(0, sBuffer.IndexOf(' ')).ToUpper();
                        }
                    }
                    catch { mySocket.Close(); break; }

                    if (getUrl == "")
                        break; ;

                    //pega os dados que são enviados por post
                    //verifica se já possui todo o conteudo
                    if (method != Methods.UNKNOWN)
                    {

                        status = 200;

                        msg = null;
                        binaryResp = null;
                        if (this.onClienteDataSend != null)
                        {
                            msg = this.onClienteDataSend(method, getUrl, strHeader, body, out binaryResp, out mime, out status, out opHeaders, mySocket.Client);
                        }

                        if ((conf_autoSendFiles) && ((getUrl.ToLower().IndexOf("file=") > -1) || (getUrl == "/") || (getUrl.ToLower() == "/favicon.ico") || (getUrl.ToLower() == "/index.htm") || (getUrl.ToLower() == "/index.html")) && (binaryResp == null) && (msg == null))
                        {
                            loadFile(getUrl, out binaryResp, out mime, out status);
                        }



                        if (binaryResp == null)
                        {
                            if (msg == null)
                                msg = "";

                            if (onContentReady != null)
                            {
                                byte[] temp = onContentReady(method, getUrl, body, "text/html", System.Text.Encoding.Default.GetBytes(msg), mySocket.Client);
                                if (temp != null)
                                    msg = System.Text.Encoding.Default.GetString(temp);
                            }

                            if (status == 0)
                                status = 200;

                            if (mime == "")
                                mime = "text/html; charset=UTF-8";

                            if (opHeaders != "")
                                opHeaders = "\r\n" + opHeaders;

                            dataSend = "HTTP/1.1 " + status.ToString() + " " + getHttpCodeDescription(status) + "\r\n" +
                                                                "Server: Kiwiisco Webserver 1.1, embedded version\r\n" +
                                                                "Content-Type: " + mime + "\r\n" +
                                                                "Content-Length: " + Convert.ToString(Encoding.UTF8.GetByteCount(msg)) + "\r\n" +
                                                                "Accept-Ranges: bytes\r\n" +
                                                                "Connection: " + (this.conf_keepAlive ? "Keep-Alive" : "Close") +
                                                                opHeaders + "\r\n\r\n" + msg;

                            mime = "text/html";
                            buffer = Encoding.UTF8.GetBytes(dataSend);
                        }
                        else
                        {
                            if (onContentReady != null)
                            {
                                byte[] temp = onContentReady(method, getUrl, body, mime, binaryResp, mySocket.Client);
                                if (temp != null)
                                    binaryResp = temp;
                            }

                            /*status = 401;
                            "WWW-Authenticate:	Basic realm=\"TP-LINK Wireless N Router WR841N\"" +*/
                            if (opHeaders != "")
                                opHeaders = "\r\n" + opHeaders;

                            dataSend = "HTTP/1.1 " + status.ToString() + " " + getHttpCodeDescription(status) + "\r\n" +
                                                                "Server: Kiwiisco Webserver 1.1, embedded version\r\n" +
                                                                "Content-Type: " + mime + "\r\n" +
                                                                "Content-Length: " + Convert.ToString(binaryResp.Length) + "\r\n" +
                                                                "Accept-Ranges: bytes\r\n" +
                                                                "Connection: " + (this.conf_keepAlive ? "Keep-Alive" : "Close") +
                                                                opHeaders + "\r\n\r\n";


                            //coloca os dados binários no buffer
                            buffer = new byte[dataSend.Length + binaryResp.Length];
                            for (cont = 0; cont < dataSend.Length; cont++)
                                buffer[cont] = Convert.ToByte(dataSend[cont]);

                            for (cont = 0; cont < binaryResp.Length; cont++)
                                buffer[cont + dataSend.Length] = binaryResp[cont];



                        }


                        if (onReadyToSend != null)
                        {
                            byte[] temp = onReadyToSend(method, getUrl, body, mime, buffer, mySocket.Client);
                            if (temp != null)
                                buffer = temp;
                        }

                        try
                        {
                            sslStream.Write(buffer, 0, buffer.Length);
                        }
                        finally
                        {
                            try
                            {
                                mySocket.Close();
                            }
                            catch { }
                        }

                    }
                    Thread.Sleep(10);
                }
            }
            catch (AuthenticationException e)
            {
                Console.WriteLine("Exception: {0}", e.Message);
                if (e.InnerException != null)
                {
                    Console.WriteLine("Inner exception: {0}", e.InnerException.Message);
                }
                Console.WriteLine("Authentication failed - closing the connection.");
                sslStream.Close();
                mySocket.Close();
                return;
            }
            catch (Exception e)
            {
                Console.WriteLine("Exception: {0}", e.Message);
                if (e.InnerException != null)
                {
                    Console.WriteLine("Inner exception: {0}", e.InnerException.Message);
                }
                Console.WriteLine("Authentication failed - closing the connection.");
                sslStream.Close();
                mySocket.Close();
                return;
            }

            if (mySocket.Connected)
            {
                mySocket.Close();
            }
        }

        private string ReadMessage(Stream sslStream)
        {
            // Read the  message sent by the client.
            // The client signals the end of the message using the
            // "<EOF>" marker.
            byte[] buffer = new byte[2048];
            StringBuilder messageData = new StringBuilder();
            int bytes = -1;
            do
            {
                try
                {
                    // Read the client's test message.
                    bytes = sslStream.Read(buffer, 0, buffer.Length);

                    // Use Decoder class to convert from bytes to UTF8
                    // in case a character spans two buffers.
                    Decoder decoder = Encoding.UTF8.GetDecoder();
                    char[] chars = new char[decoder.GetCharCount(buffer, 0, bytes)];
                    decoder.GetChars(buffer, 0, bytes, chars, 0);
                    messageData.Append(chars);
                    // Check for EOF or an empty message.
                    /*if (messageData.ToString().IndexOf("<EOF>") != -1)
                    {
                        break;
                    }*/
                }
                catch { break; }
            } while (bytes != 0);

            return messageData.ToString();
        }

        Dictionary<int, string> translates = null;
        private string getHttpCodeDescription(int httpCode)
        {
            //só carrega a lista da primeira vez
            if (translates == null)
            {
                translates = new Dictionary<int, string>();

                translates.Add(100, "Continue");
                translates.Add(101, "Switching Protocols");
                translates.Add(200, "OK");
                translates.Add(201, "Created");
                translates.Add(202, "Accepted");
                translates.Add(203, "Non-Authoritative Information");
                translates.Add(204, "No Content");
                translates.Add(205, "Reset Content");
                translates.Add(206, "Partial Content");
                translates.Add(300, "Multiple Choices");
                translates.Add(301, "Moved Permanently");
                translates.Add(302, "Found");
                translates.Add(303, "See Other");
                translates.Add(304, "Not Modified");
                translates.Add(305, "Use Proxy");
                translates.Add(306, "(Unused)");
                translates.Add(307, "Temporary Redirect");
                translates.Add(400, "Bad Request");
                translates.Add(401, "Unauthorized");
                translates.Add(402, "Payment Required");
                translates.Add(403, "Forbidden");
                translates.Add(404, "Not Found");
                translates.Add(405, "Method Not Allowed");
                translates.Add(406, "Not Acceptable");
                translates.Add(407, "Proxy Authentication Required");
                translates.Add(408, "Request Timeout");
                translates.Add(409, "Conflict");
                translates.Add(410, "Gone");
                translates.Add(411, "Length Required");
                translates.Add(412, "Precondition Failed");
                translates.Add(413, "Request Entity Too Large");
                translates.Add(414, "Request-URI Too Long");
                translates.Add(415, "Unsupported Media Type");
                translates.Add(416, "Requested Range Not Satisfiable");
                translates.Add(417, "Expectation Failed");
                translates.Add(500, "Internal Server Error");
                translates.Add(501, "Not Implemented");
                translates.Add(502, "Bad Gateway");
                translates.Add(503, "Service Unavailable");
                translates.Add(504, "Gateway Timeout");
                translates.Add(505, "HTTP Version Not Supported");
            }

            if (translates.Keys.ToList().IndexOf(httpCode) > -1)
                return translates[httpCode];
            else
                return "Other";


        }
    }
}

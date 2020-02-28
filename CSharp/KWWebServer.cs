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
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace KW
{
    public abstract class HTTP_CODES
    {
        public const int Continue = 100;
        public const int SwitchingProtocols = 101;
        public const int Processing = 102;//webdav
        public const int EarlyHints = 103;
        public const int OK = 200;
        public const int Created = 201;
        public const int Accepted = 202;
        public const int NonAuthoritativeInformation = 203;
        public const int NoContent = 204;
        public const int ResetContent = 205;
        public const int PartialContent = 206;
        public const int MultiStatus = 207; //webdav
        public const int AlreadyReported = 208; //webdav
        public const int IMUsed= 226;
        public const int MultipleChoices = 300;
        public const int MovedPermanently = 301;
        public const int Found = 302;
        public const int SeeOther = 303;
        public const int NotModified = 304;
        public const int UseProxy = 305;
        public const int Unused = 306;
        public const int TemporaryRedirect = 307;
        public const int PermanentRedirect = 308;
        public const int BadRequest = 400;
        public const int Unauthorized = 401;
        public const int PaymentRequired = 402;
        public const int Forbidden = 403;
        public const int NotFound = 404;
        public const int MethodNotAllowed = 405;
        public const int NotAcceptable = 406;
        public const int ProxyAuthenticationRequired = 407;
        public const int RequestTimeout = 408;
        public const int Conflict = 409;
        public const int Gone = 410;
        public const int LengthRequired = 411;
        public const int PreconditionFailed = 412;
        public const int RequestEntityTooLarge = 413;
        public const int RequestURITooLong = 414;
        public const int UnsupportedMediaType = 415;
        public const int RequestedRangeNotSatisfiable = 416;
        public const int ExpectationFailed = 417;
        public const int IamTeaport = 418;
        public const int MisdirectedRequest = 421;
        public const int UmprocessableEntity = 422; //webDav
        public const int Locked = 423; //webDav
        public const int FailedDependency = 424;
        public const int TooEarly = 425;
        public const int UpgradeRequired = 426;
        public const int PreconditionRequired = 428;
        public const int TooManyRequests = 429;
        public const int RequestHeaderFieldsTooLarge = 431;
        public const int UnavailableForLegalRasons = 451;
        public const int InternalServerError = 500;
        public const int NotImplemented = 501;
        public const int BadGateway = 502;
        public const int ServiceUnavailable = 503;
        public const int GatewayTimeout = 504;
        public const int HTTPVersionNotupported = 505;
        public const int VariantAlsoNegotiates = 506;
        public const int InsufficientStorage = 507;
        public const int LoopDetected = 508;
        public const int NotExtended = 510;
        public const int NetworkAuthenticationRequired = 511;
    };

    public class HttpSessionData : IDisposable
    {
        public string method = "GET";
        public string resource = "";
        public Dictionary<string, string> headers = new Dictionary<string, string>();
        public byte[] body = new byte[0];
        public string mime = "";
        public int httpStatus = 200;
        public Socket tcpClient;

        public string sBody
        {
            get { return Encoding.UTF8.GetString(this.body); }
            set { this.body = Encoding.UTF8.GetBytes(value); }

        }

        public void Dispose()
        {
            method = "";
            resource = "";
            headers?.Clear();
            headers = null;
            body = new byte[0];
            mime = "";
            tcpClient = null;
        }
    }

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
        public delegate HttpSessionData onDataEvent(HttpSessionData data);
        public delegate string OnWebSocketData(string resource, object wsId, string stringData, byte[] rawData, out byte[] opRawReturn);
        public delegate void OnWebSocketConnectedDisconnected(string resource, object wsId);

        public event onDataEvent onClienteDataSend;
        public event OnWebSocketData onWebSocketData;

        public event OnWebSocketConnectedDisconnected onWebSocketConnected;
        public event OnWebSocketConnectedDisconnected onWebSocketDisconnected;


        private bool conf_autoSendFiles;
        public List<string> conf_filesFolders = new List<string>();
        private bool conf_multiThread = true;
        private bool conf_keepAlive = false;
        private string conf_defaultCharSet = "UTF-8";
        private bool rodando;
        private List<Thread> ths = new List<Thread>();

        List<Stream> webSocketStreams = new List<Stream>();

        List<TcpListener> listeners = new List<TcpListener>();

        public delegate void OnReadyToSendDelegate(ref HttpSessionData output);


        //retornar null para evitar processamento excessivo (executado antes de receber o cabeçalho)
        public OnReadyToSendDelegate onContentReady;

        //retornar null para evitar processamento excessivo (executado antes de enviar o conteúdo para o browser)
        public OnReadyToSendDelegate onReadyToSend;




        public KWHttpServer(int[] ports, bool pconf_autoSendFiles, List<string> conf_autoSendFiles_fodler, bool use_multiThreads = true, string defaultCharSet = "UTF-8", bool useKeepAliveConnection = false)
        {
            this.conf_defaultCharSet = defaultCharSet;
            this.conf_multiThread = use_multiThreads;
            this.conf_autoSendFiles = pconf_autoSendFiles;
            this.conf_filesFolders = conf_autoSendFiles_fodler;
            this.conf_keepAlive = useKeepAliveConnection;

            for (int cont = 0; cont < conf_filesFolders.Count; cont++)
            {
                if ((this.conf_filesFolders[cont].Length > 0) && (this.conf_filesFolders[cont][this.conf_filesFolders[cont].Length - 1] != '\\'))
                    this.conf_filesFolders[cont] += '/';
            }

            try
            {


                //start listing on the given port
                foreach (var c in ports)
                {
                    listeners.Add(new TcpListener(IPAddress.Any, c));
                    listeners.Last().Start();

                    //start the thread which calls the method 'StartListen'
                    ths.Add(new Thread(delegate (object args)
                    {
                        StartListen((TcpListener)args);
                    }));
                    ths.Last().Start(listeners.Last());
                }

            }
            catch
            {
            }

        }

        public bool enableHttps(string pfxCert, string pfxPass)
        {
            try
            {
                serverCertificate = new X509Certificate2(pfxCert, pfxPass);//@"C:\Users\engenharia4\Desktop\https\dinv001.pfx", "inovadaq");//X509Certificate2.CreateFromCertFile(@"C:\Users\engenharia4\Desktop\https\server.pfx", "inovadaq");
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

            mime = "text/html" + (this.conf_defaultCharSet != "" ? "; charset=" + this.conf_defaultCharSet : "");
            if (getUrl.IndexOf('?') > -1)
                getUrl = getUrl.Split('?')[0];



            binaryResp = null;
            try
            {
                httpStatus = 200;

                int cont = 0;
                while ((binaryResp == null) && (cont < conf_filesFolders.Count))
                {
                    var conf_filesFolder = conf_filesFolders[cont];
                    string getUrlCurrent = getUrl;
                    string currentFIlename = "";
                    if (getUrlCurrent == "/")
                    {
                        mime = "text/html" + (this.conf_defaultCharSet != "" ? "; charset=" + this.conf_defaultCharSet : ""); ;
                        if (File.Exists(conf_filesFolder + "index.htm"))
                            binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + "index.htm");
                        else
                            if (File.Exists(conf_filesFolder + "index.html"))
                            binaryResp = System.IO.File.ReadAllBytes(conf_filesFolder + "index.html");
                    }
                    else
                    {
                        if (getUrlCurrent.ToUpper().IndexOf("FILE=") == -1)
                        {
                            if ((getUrlCurrent.Length > 0) && (getUrlCurrent[0] == '/'))
                                getUrlCurrent = getUrlCurrent.Substring(1);

                            if ((getUrlCurrent.Length > 0) && (getUrlCurrent[getUrlCurrent.Length - 1] == '/'))
                                getUrlCurrent = getUrlCurrent.Substring(0, getUrlCurrent.Length - 1);

                            if (Directory.Exists(conf_filesFolder + getUrlCurrent))
                            {
                                getUrlCurrent = getUrlCurrent + "/index.htm";
                                mime = "text/html" + (this.conf_defaultCharSet != "" ? "; charset=" + this.conf_defaultCharSet : ""); ;
                            }


                            getUrlCurrent = "file=" + getUrlCurrent;
                        }

                        currentFIlename = conf_filesFolder + getUrlCurrent.Split('=')[1];
                        if (System.IO.File.Exists(conf_filesFolder + getUrlCurrent.Split('=')[1]))
                        {
                            if (getUrlCurrent.IndexOf(".gif") > -1)
                                mime = "image/gif";
                            else if (getUrlCurrent.IndexOf(".jpg") > -1)
                                mime = "image/jpeg";
                            else if (getUrlCurrent.IndexOf(".png") > -1)
                                mime = "image/png";
                            else if (getUrlCurrent.IndexOf(".json") > -1)
                                mime = "application/json" + (this.conf_defaultCharSet != "" ? "; charset=" + this.conf_defaultCharSet : "");
                            else if (getUrlCurrent.IndexOf(".js") > -1)
                                mime = "application/javascript";
                            else if (getUrlCurrent.IndexOf(".css") > -1)
                                mime = "text/css";
                            else
                                mime = "text/html;" + (this.conf_defaultCharSet != "" ? "; charset=" + this.conf_defaultCharSet : ""); ;// "text/html";

                            binaryResp = System.IO.File.ReadAllBytes(currentFIlename);
                        }
                    }

                    cont++;
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

        //a função abaixo retorna se a requisição trata-se de um arquivo válido
        private bool isValidFileRequest(string getUrl)
        {
            if ((getUrl.ToLower().IndexOf("file=") > -1) || (getUrl == "/") || (getUrl.ToLower() == "/favicon.ico") || (getUrl.ToLower() == "/index.htm") || (getUrl.ToLower() == "/index.html"))
                return true;

            if (getUrl.IndexOf('?') > -1)
                getUrl = getUrl.Split('?')[0];




            if ((getUrl.Length > 0) && (getUrl[0] == '/'))
                getUrl = getUrl.Substring(1);
            if ((getUrl.Length > 0) && (getUrl[getUrl.Length - 1] == '/'))
                getUrl = getUrl.Substring(0, getUrl.Length - 1);

            int cont = 0;
            while (cont < conf_filesFolders.Count)
            {
                string getUrlCurrent = conf_filesFolders[cont] + getUrl;
                if (File.Exists(getUrlCurrent) == true)
                {
                    return true;
                }
                else
                {
                    if (Directory.Exists(getUrlCurrent))
                    {
                        getUrlCurrent = getUrlCurrent + "/index.htm";
                        if (File.Exists(getUrlCurrent))
                            return true;
                    }
                }
                cont++;
            }

            return false;
        }


        public void StopWork()
        {
            rodando = false;

            System.Threading.Thread.Sleep(100);
            foreach (var c in this.listeners)
            {
                if (c != null)
                {
                    c.Stop();
                }
            }

            foreach (var c in ths)
                c.Interrupt();
        }

        //List<Thread> threads = new List<Thread>();

        private void StartListen(TcpListener listener)
        {
            rodando = true;

            bool acceptOk = true;
            while (rodando)
            {
                //Accept a new connection
                if (listener == null)
                    break;


                if (acceptOk)
                {
                    acceptOk = false;
                    listener.BeginAcceptTcpClient(delegate (IAsyncResult ar)
                    {
                        acceptOk = true;
                        TcpListener listener2 = (TcpListener)ar.AsyncState;
                        if (rodando)
                        {
                            TcpClient client = listener2.EndAcceptTcpClient(ar);

                            //escutaCliente(client, 0);

                            //Thread temp = new Thread(delegate (object threadPointer) {});
                            Action<TcpClient> ac = (clientP) => {
                                Task t = new Task(delegate ()
                                {
                                    escutaCliente(clientP);

                                });

                                t.Start();
                            };
                            ac.Invoke(client);

                            //listener = null;
                            //client = null;
                            //temp = null;
                        }


                    }, listener);
                }


                Thread.Sleep(1);
            }
            return;
        }

        public void escutaCliente(TcpClient mySocket)
        {

            HttpSessionData input = new HttpSessionData();
            HttpSessionData output = new HttpSessionData
            {
                httpStatus = 200,
                sBody = "",
                mime = "text/html" + (this.conf_defaultCharSet != "" ? "; charset=" + this.conf_defaultCharSet : "")
            };

            string dataSend = "";
            string sBuffer = "";
            string webSocketKey = "";
            string strHeader = "";
            string novosDados;
            string estado;

            string[] headersArrTemp;

            byte[] buffer = null;

            int contentLength = 0;
            int contentStart = 0;
            int cont;
            int status = 200;
            int readTimeout = 10000;

            Dictionary<string, string> headers = new Dictionary<string, string>();

            Stream sslStream = null;

            mySocket.ReceiveTimeout = 2500;

            if (useHttps)
                sslStream = new SslStream(mySocket.GetStream(), false);
            else
                sslStream = new NetworkStream(mySocket.Client, true);

            //o navegador google chrome, as vezes, demora para abrir a conexão e enviar os dados. Caso isso aconteca, aguarda um segundo

            readTimeout = 1000;
            while ((mySocket == null) || (mySocket.Client == null) || (!mySocket.Client.Connected))
            {
                readTimeout--;
                System.Threading.Thread.Sleep(10);
                readTimeout--;

                if (readTimeout <= 0)
                    return;
            }

            readTimeout = 1000;

            estado = "identificandoHttps";

            while ((rodando) && (mySocket != null) && (mySocket.Client != null) && (mySocket.Client.Connected))
            {
                switch (estado)
                {
                    #region identificandoHttps
                    case "identificandoHttps":
                        try
                        {
                            if (useHttps)
                            {
                                ((SslStream)sslStream).AuthenticateAsServer(serverCertificate, false, SslProtocols.Tls | SslProtocols.Tls | SslProtocols.Ssl3 | SslProtocols.Ssl3, true);
                            }
                        }
                        catch (Exception e) { estado = e.Message; }
                        estado = "lerDadosCabecalho";
                        break;
                    #endregion
                    #region lerDadosConteudo, lerDadosCabecalho
                    case "lerDadosConteudo":
                    case "lerDadosCabecalho":

                        readTimeout = 1000;
                        sBuffer = "";
                        System.Threading.Thread.Sleep(1);
                        if (estado == "lerDadosCabecalho")
                            estado = "lendoDadosCabecalho";
                        else
                            estado = "lendoDadosConteudo";
                        break;
                    #endregion
                    #region lendoDadosConteudo, lendoDadosCabecalho
                    case "lendoDadosConteudo":
                    case "lendoDadosCabecalho":
                        System.Threading.Thread.Sleep(1);
                        readTimeout -= 10;
                        //verifica se deu timeout
                        if (readTimeout < 0)
                        {
                            estado = "erroTimeout";
                            continue;
                        }



                        sslStream.ReadTimeout = 100;
                        sslStream.WriteTimeout = 100;

                        novosDados = ReadMessage(sslStream, mySocket);

                        if (novosDados == "")
                        {
                            continue;
                        }


                        sBuffer += novosDados;


                        //verifica se já veio algo

                        if (estado == "lendoDadosCabecalho")
                            estado = "processaCabecalho";
                        else
                            estado = "processaConteudo";

                        break;
                    #endregion
                    #region processaCabecalho
                    case "processaCabecalho":
                        contentStart = 0;

                        //verifica se já recebeu todo o cabeçalho
                        if (sBuffer.IndexOf("\r\n\r\n") > -1)
                            strHeader = sBuffer.Substring(0, sBuffer.IndexOf("\r\n\r\n"));
                        else if (sBuffer.IndexOf("\n\n") > -1)
                            strHeader = sBuffer.Substring(0, sBuffer.IndexOf("\n\n"));
                        else
                        {

                            //continua lendo dados
                            estado = "lendoDadosCabecalho";
                            continue;
                        }


                        contentStart = sBuffer.IndexOf("\r\n\r\n") + 4;
                        if (contentStart == 3)
                            contentStart = sBuffer.IndexOf("\n\n") + 2;

                        //coloca o cabeçalho em um dicionário
                        headers.Clear();
                        strHeader = strHeader.Replace("\r", "");
                        headersArrTemp = strHeader.Split('\n');
                        foreach (var att in headersArrTemp)
                        {
                            if (att.IndexOf(':') > -1)
                            {
                                headers[att.Split(':')[0].ToLower()] = att.Split(':')[1];
                                if (headers.Last().Value[0] == ' ')
                                    headers[headers.Last().Key] = headers.Last().Value.Substring(1);
                            }
                        }


                        input.resource = sBuffer.Substring(sBuffer.IndexOf(' ') + 1);// + 1, sBuffer.IndexOf('\n') - sBuffer.ToUpper().IndexOf(" HTTP"));
                        input.resource = input.resource.Substring(0, input.resource.ToUpper().IndexOf(" HTTP"));
                        input.resource = System.Uri.UnescapeDataString(input.resource);

                        input.method = sBuffer.Substring(0, sBuffer.IndexOf(' ')).ToUpper();

                        //identifica uma conexão websocks
                        if (headers.ContainsKey("sec-websocket-key"))
                        {
                            webSocketKey = headers["sec-websocket-key"];
                            estado = "inicializaWebSocks";
                            continue;
                        }


                        //verifica se possui o "content-length"
                        if (headers.ContainsKey("content-length"))
                        {
                            contentLength = Int32.Parse(headers["content-length"]);
                            estado = "processaConteudo";
                        }
                        else
                            estado = "processaRequisicao";

                        break;
                    #endregion
                    #region processaConteudo
                    case "processaConteudo":


                        if ((Encoding.UTF8.GetByteCount(sBuffer) - contentStart) < contentLength)
                        {


                            estado = "lendoDadosConteudo";
                            continue;
                        }
                        //pega a url requisitada
                        input.sBody = sBuffer.Substring(contentStart);

                        estado = "processaRequisicao_levantamentoDeDados";
                        break;
                    #endregion
                    #region processaRequisicao, processaRequisicao_levantamentoDeDados
                    case "processaRequisicao":
                    case "processaRequisicao_levantamentoDeDados":
                        if (input.method != Methods.UNKNOWN)
                        {

                            status = 200;

                            if (this.onClienteDataSend != null)
                            {
                                output = this.onClienteDataSend(input);
                            }

                            if ((conf_autoSendFiles) && (isValidFileRequest(input.resource)) && ((output.body == null) || (output.body.Length == 0)))
                            {
                                loadFile(input.resource, out output.body, out output.mime, out output.httpStatus);
                            }

                            estado = "processaRequisicao_preparaResposta";
                        }
                        else
                            estado = "erroMethodoDesconhecido";
                        break;
                    #endregion 
                    #region processaRequisicao_preparaResposta
                    case "processaRequisicao_preparaResposta":


                        if (output.body == null)
                            output.body = new byte[0];

                        if (onContentReady != null)
                            onContentReady(ref output);

                        if (status == 0)
                            status = 200;


                        if (output.mime == "")
                            output.mime = "text/html";

                        if (!output.mime.ToLower().Contains("charset") && this.conf_defaultCharSet != "")
                            output.mime += "; charset=" + this.conf_defaultCharSet;


                        dataSend = "HTTP/1.1 " + status.ToString() + " " + getHttpCodeDescription(output.httpStatus) + "\r\n" +
                            "Server: " + (output.headers.ContainsKey("Server") ? output.headers["Server"] : "Kiwiisco Webserver 1.1, embedded version\r\n") +
                            "Content-Type: " + output.mime + "\r\n" +
                            "Content-Length: " + output.body.Length + "\r\n" +
                            "Accept-Ranges: bytes\r\n" +
                            "Connection: " + (this.conf_keepAlive ? "Keep-Alive" : "Close") + "\r\n";
                        //add coustom headers

                        foreach (var c in output.headers)
                            dataSend += c + "\r\n";

                        //add header and content separador
                        dataSend += "\r\n";

                        buffer = new byte[dataSend.Length + output.body.Length];
                        //put the dataSend bytes to buffer
                        for (cont = 0; cont < dataSend.Length; cont++)
                            buffer[cont] = Convert.ToByte(dataSend[cont]);

                        //put the content bbytes to buffer
                        for (cont = 0; cont < output.body.Length; cont++)
                            buffer[cont + dataSend.Length] = output.body[cont];

                        estado = "enviarResposta";
                        break;
                    #endregion
                    #region enviarResposta
                    case "enviarResposta":


                        if (onReadyToSend != null)
                            onReadyToSend(ref output);

                        try
                        {
                            sslStream.Write(buffer, 0, buffer.Length);
                        }
                        catch { }
                        estado = "finalizaRequisicao";
                        break;
                    #endregion
                    #region inicializaWebSocks
                    case "inicializaWebSocks":
                        //envia uma resposta para o navegador
                        Byte[] response = Encoding.UTF8.GetBytes("HTTP/1.1 101 Switching Protocols" + Environment.NewLine
                        + "Connection: Upgrade" + Environment.NewLine
                        + "Upgrade: websocket" + Environment.NewLine
                        + "Sec-WebSocket-Accept: " + Convert.ToBase64String(
                            SHA1.Create().ComputeHash(
                                Encoding.UTF8.GetBytes(
                                    //new Regex("Sec-WebSocket-Key: (.*)").Match(webSocketKey).Groups[1].Value.Trim() + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
                                    webSocketKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
                                )
                            )
                        ) + Environment.NewLine
                        + Environment.NewLine);

                        sslStream.Write(response, 0, response.Length);


                        webSocketStreams.Add(sslStream);
                        if (onWebSocketConnected != null)
                            onWebSocketConnected(input.resource, sslStream);

                        estado = "lerDadosWebSocks";
                        break;
                    #endregion
                    #region lerDadosWebSocks
                    case "lerDadosWebSocks":

                        Byte[] encoded = ReadMessageBytes(sslStream, mySocket);
                        byte[] decoded;
                        byte[] resultRawData;
                        if (encoded.Length > 0)
                        {
                            novosDados = GetWebSocketDecodedData(encoded, encoded.Length, out decoded);

                            if (onWebSocketData != null)
                            {
                                string msg = onWebSocketData(input.resource, sslStream, novosDados, decoded, out resultRawData);
                                if ((msg != null) && (msg != ""))
                                {
                                    sendWebSocketData(sslStream, msg);
                                }
                                else if ((resultRawData != null) && (resultRawData.Length > 0))
                                {
                                    sendWebSocketData(sslStream, resultRawData);
                                }
                            }
                        }
                        else
                            Thread.Sleep(1); //só aguarda caso não venha nada

                        break;
                    #endregion
                    #region erroTimeout
                    case "erroTimeout":
                        estado = "finalizaRequisicao";
                        break;
                    #endregion
                    #region erroMethodoDesconhecido
                    case "erroMethodoDesconhecido":
                        estado = "finalizaRequisicao";
                        break;
                    #endregion
                    #region finalizaRequisicao
                    case "finalizaRequisicao":
                        try
                        {
                            if (webSocketStreams.Contains(sslStream))
                            {
                                webSocketStreams.Remove(sslStream);
                                if (onWebSocketDisconnected != null)
                                    onWebSocketDisconnected(input.resource, sslStream);
                            }
                        }
                        catch { }

                        try
                        {
                            mySocket.Close();
                            sslStream.Close();
                            sslStream.Dispose();
                            buffer = null;
                            output.Dispose();
                            input.Dispose();
                            dataSend = null;
                            sBuffer = null;
                            webSocketKey = null;
                            strHeader = null;
                            novosDados = null;
                            estado = null;

                            //threads.Remove(threadPointer);
                        }
                        catch { }
                        break;
                        #endregion
                }
            }

            try
            {
                if (webSocketStreams.Contains(sslStream))
                {
                    webSocketStreams.Remove(sslStream);
                    if (onWebSocketDisconnected != null)
                        onWebSocketDisconnected(input.resource, sslStream);
                }
            }
            catch { }

            buffer = null;
            dataSend = null;
            sBuffer = null;
            webSocketKey = null;
            strHeader = null;
            novosDados = null;
            estado = null;
            input.Dispose();
            output.Dispose();
        }

        public static string GetWebSocketDecodedData(byte[] buffer, int length, out byte[] rawDecodedData)
        {
            byte b = buffer[1];
            int dataLength = 0;
            int totalLength = 0;
            int keyIndex = 0;

            if (b - 128 <= 125)
            {
                dataLength = b - 128;
                keyIndex = 2;
                totalLength = dataLength + 6;
            }

            if (b - 128 == 126)
            {
                dataLength = BitConverter.ToInt16(new byte[] { buffer[3], buffer[2] }, 0);
                keyIndex = 4;
                totalLength = dataLength + 8;
            }

            if (b - 128 == 127)
            {
                dataLength = (int)BitConverter.ToInt64(new byte[] { buffer[9], buffer[8], buffer[7], buffer[6], buffer[5], buffer[4], buffer[3], buffer[2] }, 0);
                keyIndex = 10;
                totalLength = dataLength + 14;
            }

            if (totalLength > length)
                throw new Exception("The buffer length is small than the data length");

            byte[] key = new byte[] { buffer[keyIndex], buffer[keyIndex + 1], buffer[keyIndex + 2], buffer[keyIndex + 3] };

            int dataIndex = keyIndex + 4;
            int count = 0;
            for (int i = dataIndex; i < totalLength; i++)
            {
                buffer[i] = (byte)(buffer[i] ^ key[count % 4]);
                count++;
            }

            rawDecodedData = new byte[dataLength];
            for (int i = dataLength; i < dataLength; i++)
                rawDecodedData[i - dataLength] = buffer[i];

            return Encoding.ASCII.GetString(buffer, dataIndex, dataLength);
        }

        /// <summary>
        /// Send data to a WebSock. Sends webSocketStrem as null to broadcast
        /// </summary>
        /// <param name="webSocketStream">TR: O Stream do web socket (wsId passado no evento onWebSocketData). Se for null, a mensagem é enviada por broadcast (para todos os websockets conectados).(</param>
        /// <param name="data">Os dados a serem enviados</param>
        public bool sendWebSocketData(object webSocketStream, byte[] data)
        {

            List<byte> lb = new List<byte>();
            byte[] sizeBytes;
            lb.Add(0x81);
            //de 0 a 125 é o tamanho do pacote. Se for 126, os próximos 2 bytes são um word com o tamanho. Se for 127 então os 8 bytes seguintes são um Int64 com o tamanho do pacote
            if (data.Length < 126)
                lb.Add((byte)data.Length);
            else if (data.Length < 65536)
            {
                Int16 size = (Int16)(data.Length);
                lb.Add(126);
                sizeBytes = BitConverter.GetBytes(size);
                lb.Add(sizeBytes[1]);
                lb.Add(sizeBytes[0]);
            }
            else
            {
                Int64 size = data.Length;
                lb.Add(127);
                sizeBytes = BitConverter.GetBytes(size);
                lb.Add(sizeBytes[7]);
                lb.Add(sizeBytes[6]);
                lb.Add(sizeBytes[5]);
                lb.Add(sizeBytes[4]);
                lb.Add(sizeBytes[3]);
                lb.Add(sizeBytes[2]);
                lb.Add(sizeBytes[1]);
                lb.Add(sizeBytes[0]);
            }
            lb.AddRange(data);
            if (webSocketStream != null)
                try
                {
                    ((Stream)webSocketStream).Write(lb.ToArray(), 0, lb.Count());
                }
                catch { return false; }
            else
            {
                int sucess = 0;
                for (int cont = 0; cont < webSocketStreams.Count; cont++)
                {
                    var att = webSocketStreams[cont];
                    try
                    {
                        att.Write(lb.ToArray(), 0, lb.Count());
                        sucess++;
                    }
                    catch { }
                }


                //if anyone respond, return false
                if (sucess == 0)
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Send data to a WebSock. Sends webSocketStrem as null to broadcast
        /// </summary>
        /// <param name="webSocketStream">TR: O Stream do web socket (wsId passado no evento onWebSocketData). Se for null, a mensagem é enviada por broadcast (para todos os websockets conectados).(</param>
        /// <param name="data">Os dados a serem enviados</param>
        public void sendWebSocketData(object webSocketStream, string data)
        {
            this.sendWebSocketData(webSocketStream, Encoding.UTF8.GetBytes(data));
        }



        private string ReadMessage(Stream sslStream, TcpClient client)
        {
            // Read the  message sent by the client.
            // The client signals the end of the message using the
            // "<EOF>" marker.
            if (useHttps)
            {
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
                string ret = messageData.ToString();
                messageData.Clear();
                return ret;
            }
            else
            {
                Byte[] bReceive;// = new Byte[1024];
                string sBuffer = "";
                try
                {

                    int i = 1;

                    while (/*(i > 0) &&*/ (client.Client.Available > 0))
                    {
                        try
                        {
                            bReceive = new byte[client.Client.Available];

                            i = client.Client.Receive(bReceive, client.Client.Available, 0);
                            sBuffer += Encoding.UTF8.GetString(bReceive);

                            //reinicia o timeout, para, por exemplo, quando se está fazendo um upload
                        }
                        catch { break; }
                    }


                    bReceive = new byte[] { };
                    return sBuffer;
                }
                catch
                {
                    return "";
                }


            }
        }

        private byte[] ReadMessageBytes(Stream sslStream, TcpClient client)
        {
            // Read the  message sent by the client.
            // The client signals the end of the message using the
            // "<EOF>" marker.
            List<byte> retorno = new List<byte>();
            if (useHttps)
            {
                byte[] buffer = new byte[2048];
                int bytes = -1;
                do
                {
                    try
                    {
                        // Read the client's test message.
                        bytes = sslStream.Read(buffer, 0, buffer.Length);

                        for (int a = 0; a < bytes; a++)
                            retorno.Add(buffer[a]);
                    }
                    catch { break; }
                } while (bytes != 0);

                return retorno.ToArray();
            }
            else
            {
                byte[] bReceive = new byte[] { };// = new Byte[1024];
                try
                {

                    int i = 1;

                    while (/*(i > 0) &&*/ (client.Client.Available > 0))
                    {
                        try
                        {
                            bReceive = new byte[client.Client.Available];

                            i = client.Client.Receive(bReceive, client.Client.Available, 0);


                            //reinicia o timeout, para, por exemplo, quando se está fazendo um upload
                        }
                        catch { break; }
                    }

                    return bReceive;
                }
                catch
                {
                    return new byte[] { };
                }


            }
        }

        Dictionary<int, string> translates = new Dictionary<int, string> {
            {100, "Continue"},
            {101, "Switching Protocols"},
            {102, "Processing"},
            {103, "Early Hints"},
            {200, "OK"},
            {201, "Created"},
            {202, "Accepted"},
            {203, "Non-Authoritative Information"},
            {204, "No Content"},
            {205, "Reset Content"},
            {206, "Partial Content"},
            {207, "Multi Status"},
            {208, "Already Reported"},
            {226, "IM Used"},
            {300, "Multiple Choices"},
            {301, "Moved Permanently"},
            {302, "Found"},
            {303, "See Other"},
            {304, "Not Modified"},
            {305, "Use Proxy"},
            {306, "(Unused)"},
            {307, "Temporary Redirect"},
            {308, "Permanent Redirect"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {402, "Payment Required"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {406, "Not Acceptable"},
            {407, "Proxy Authentication Required"},
            {408, "Request Timeout"},
            {409, "Conflict"},
            {410, "Gone"},
            {411, "Length Required"},
            {412, "Precondition Failed"},
            {413, "Request Entity Too Large"},
            {414, "Request-URI Too Long"},
            {415, "Unsupported Media Type"},
            {416, "Requested Range Not Satisfiable"},
            {417, "Expectation Failed"},
            {418, "IamTeaport"},
            {421, "MisdirectedRequest"},
            {422, "UmprocessableEntity"},
            {423, "Locked = 423"},
            {424, "FailedDependency"},
            {425, "TooEarly"},
            {426, "UpgradeRequired"},
            {428, "PreconditionRequired"},
            {429, "TooManyRequests"},
            {431, "RequestHeaderFieldsTooLarge"},
            {451, "UnavailableForLegalRasons"},
            {500, "Internal Server Error"},
            {501, "Not Implemented"},
            {502, "Bad Gateway"},
            {503, "Service Unavailable"},
            {504, "Gateway Timeout"},
            {505, "HTTP Version Not Supported"},
            {506, "Variant Also Negotiates"},
            {507, "Insufficient Storage"},
            {508, "Loop Detected"},
            {210, "Not Extended"},
            {511, "Network Authentication Required"},
        };

        public string getHttpCodeDescription(int httpCode)
        {
            //só carrega a lista da primeira vez
            if (translates == null)
            {

            }

            if (translates.Keys.ToList().IndexOf(httpCode) > -1)
                return translates[httpCode];
            else
                return "Other";
        }


    };


    #region router
        #region internal types
        class KWRouteItem
        {
            public string method;
            public string originalUrl;
            public List<string> urlMask;
            public onDataEvent2 onData;
        }

        public class ExtendedHttpSessionData : HttpSessionData
        {
            public Dictionary<string, string> variables = new Dictionary<string, string>();
            public ExtendedHttpSessionData(HttpSessionData importFrom)
            {
                this.method = importFrom.method;
                this.resource = importFrom.resource;
                this.headers = importFrom.headers;
                this.body = importFrom.body;
                this.mime = importFrom.mime;
                this.httpStatus = importFrom.httpStatus;
                this.tcpClient = importFrom.tcpClient;
            }
        }

        public delegate ExtendedHttpSessionData onDataEvent2(ExtendedHttpSessionData data);
        #endregion

        class KWHttpServerRouter
        {


            private List<KWRouteItem> routes = new List<KWRouteItem>();
            private onDataEvent2 defaultReq = null;

            public KWHttpServerRouter() { }
            public KWHttpServerRouter(KWHttpServer serverToAutoRoute)
            {
                serverToAutoRoute.onClienteDataSend += delegate (HttpSessionData data)
                {
                    return this.route(data);
                };
            }

            //process a incoming request. If no serverToAutoRoute is informed, this
            //method must be runned externally
            public HttpSessionData route(HttpSessionData data)
            {
                int found = 0;
                ExtendedHttpSessionData result = new ExtendedHttpSessionData(data);
                //separate url resource and url variables
                string url = data.resource.Substring(1);
                string vars = "";
                if (url.Contains('?'))
                {
                    vars = url.Substring(url.IndexOf('?') + 1);
                    url = url.Substring(0, url.IndexOf('?'));
                }

                //try identificate the requested resource
                var urlParts = url.Split('/').ToList();

                //remove emptyResources
                for (int c = urlParts.Count - 1; c >= 0; c--)
                {
                    if (urlParts[c] == "")
                        urlParts.RemoveAt(c);
                }

                foreach (var curr in this.routes)
                {
                    var valid = true;
                    if ((curr.method == "ANY" || curr.method == data.method) && (curr.urlMask.Count() == urlParts.Count || curr.originalUrl == "" || curr.originalUrl == "*"))
                    {
                        //checks if route mask is compatible with the url   
                        for (int c = 0; c < curr.urlMask.Count; c++)
                        {
                            //check if current mask position is a variable or a const
                            if (curr.urlMask[c].Length > 0 && curr.urlMask[c][0] == ':')
                            {
                                result.variables[curr.urlMask[c].Substring(1)] = urlParts[c];
                            }
                            else
                            {
                                //if current mask is a cont, checks if this value was come in the url
                                if (curr.originalUrl != "" && curr.originalUrl != "*" && curr.urlMask[c].ToUpper() != urlParts[c].ToUpper())
                                {
                                    valid = false;
                                    break;
                                }
                            }

                        }

                    }
                    else
                    {
                        valid = false;
                    }

                    if (valid)
                    {
                        found++;
                        //resolve 'URL' variables
                        var varsArr = vars.Split('&');
                        foreach (var curr2 in varsArr)
                        {
                            if (curr2.Contains('='))
                                result.variables[curr2.Substring(0, curr2.IndexOf('='))] = curr2.Substring(curr2.IndexOf('=') + 1);
                        }

                        //calls the event
                        result = curr.onData(result);

                        //prepare the result to web server
                        data.method = result.method;
                        data.resource = result.resource;
                        data.headers = result.headers;
                        data.body = result.body;
                        data.mime = result.mime;
                        data.httpStatus = result.httpStatus;
                        data.tcpClient = result.tcpClient;

                        //the system will not break here, to allow user to set more than one 'observer' to the same route
                    }
                }

                if (found == 0)
                { 
                    if (this.defaultReq != null )
                    {
                        result = this.defaultReq(result);

                        //prepare the result to web server
                        data.method = result.method;
                        data.resource = result.resource;
                        data.headers = result.headers;
                        data.body = result.body;
                        data.mime = result.mime;
                        data.httpStatus = result.httpStatus;
                        data.tcpClient = result.tcpClient;
                    }

                }

            //return the result of last found route;
            return data;
            }

            public void get(string maskedUrl, onDataEvent2 onRequest)
            {
                this.addRoute("GET", maskedUrl, onRequest);
            }

            public void post(string maskedUrl, onDataEvent2 onRequest)
            {
                this.addRoute("POST", maskedUrl, onRequest);
            }

            public void delete(string maskedUrl, onDataEvent2 onRequest)
            {
                this.addRoute("DELETE", maskedUrl, onRequest);
            }

            public void put(string maskedUrl, onDataEvent2 onRequest)
            {
                this.addRoute("PUT", maskedUrl, onRequest);
            }

            public void any(string maskedUrl, onDataEvent2 onRequest)
            {
                this.addRoute("ANY", maskedUrl, onRequest);
            }

        public void setDefault(onDataEvent2 onRequest) {
                this.defaultReq = onRequest;
            }

            public void addRoute(string method, string maskedUrl, onDataEvent2 onRequest)
            {
                this.routes.Add(new KWRouteItem
                {
                    method = method.ToUpper(),
                    urlMask = maskedUrl.Split('/').ToList(),
                    originalUrl  = maskedUrl,
                    onData = onRequest
                });
            }

        }
    #endregion

}



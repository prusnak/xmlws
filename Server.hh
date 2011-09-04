class CServer
{
	private:
		void Initialize();
		void MainLoop();
		static void *Handler(void *arg);
		static void ConnectionClose(void *arg);
		static void ErrorResponse(int cl,int code,char *uri);
		static void GETResponse(int cl,char *uri);
		static void POSTResponse(int cl,char *uri);

		int sock;
	public:
		void Start();
		static int SendBuffer(int cl,char *buf,int len);
};

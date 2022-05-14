module.exports = {
    locales: {
        '/en_us/': {
            lang: 'English',
            title: 'libfv document',
            description: 'libfv is C++20 header-only network library, support TCP/SSL/Http/websocket server and client'
        },
        '/zh_hans/': {
            lang: '简体中文',
            title: 'libfv 文档',
            description: 'libfv 是 C++20 纯头文件网络库，支持 TCP/SSL/Http/websocket 服务器端与客户端'
        }
    },
    themeConfig: {
        nav: [
            { text: 'Github', link: 'https://github.com/fawdlstty/libfv' }
        ],
        locales: {
            '/en_us/': {
                sidebar: [{
                    title: 'Home',
                    path: '/',
                    children: []
                }, {
                    title: 'Startup',
                    path: '/0_startup/',
                    children: [
                        '/'
                    ]
                }]
            },
            '/zh_hans/': {
                sidebar: [{
                    title: '首页',
                    path: '/zh_hans/',
                    children: []
                }, {
                    title: '开始使用',
                    path: '/zh_hans/0_Startup.md',
                    children: []
                }, {
                    title: 'HTTP 客户端',
                    path: '/zh_hans/1_HttpClient.md',
                    children: []
                }, {
                    title: 'Websocket 客户端',
                    path: '/zh_hans/2_WebsocketClient.md',
                    children: []
                }, {
                    title: 'TCP 及 SSL 客户端',
                    path: '/zh_hans/3_TcpSslClient.md',
                    children: []
                }, {
                    title: 'HTTP 服务器端',
                    path: '/zh_hans/4_HttpServer.md',
                    children: []
                }, {
                    title: 'TCP 及 SSL 服务器端',
                    path: '/zh_hans/5_TcpSslServer.md',
                    children: []
                }, {
                    title: '其他异步功能',
                    path: '/zh_hans/6_Other.md',
                    children: []
                }]
            }
        }
    }
}

// npm run docs:dev
// npm run docs:build

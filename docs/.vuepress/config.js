module.exports = {
    locales: {
        '/': {
            lang: 'English',
            title: 'libfv document',
            description: 'libfv is C++20 header-only network library, support TCP/SSL/Http/websocket server and client'
        },
        '/zh/': {
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
            '/': {
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
            '/zh/': {
                sidebar: [{
                    title: '首页',
                    path: '/zh/',
                    children: []
                }, {
                    title: 'Startup',
                    path: '/0_startup/',
                    children: [
                        '/'
                    ]
                }]
            }
        }
        
    }
}

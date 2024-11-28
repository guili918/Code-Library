//
//  clockApp.swift
//  clock
//
//  Created by Duan on 2024/11/28.
//

import SwiftUI

@main
struct clockApp: App {
    init() {
        // 设置支持的方向
        #if DEBUG
        // 仅在调试模式下设置方向
        if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene {
            windowScene.requestGeometryUpdate(.iOS(interfaceOrientations: .landscape))
        }
        #endif
    }
    
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

//
//  ContentView.swift
//  clock
//
//  Created by Duan on 2024/11/28.
//

import SwiftUI

struct ContentView: View {
    @State private var timeElapsed: Int = 0
    @State private var timer: Timer?
    @State private var isRunning = false
    @State private var showControls = true
    
    var timeComponents: (h1: Int, h2: Int, m1: Int, m2: Int, s1: Int, s2: Int) {
        let hours = timeElapsed / 3600
        let minutes = (timeElapsed % 3600) / 60
        let seconds = timeElapsed % 60
        
        return (
            h1: hours / 10,
            h2: hours % 10,
            m1: minutes / 10,
            m2: minutes % 10,
            s1: seconds / 10,
            s2: seconds % 10
        )
    }
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                Image("background")
                    .resizable()
                    .aspectRatio(contentMode: .fill)
                    .frame(width: geometry.size.width, height: geometry.size.height)
                    .clipped()
                    .edgesIgnoringSafeArea(.all)
                
                VStack {
                    Image("title_image")
                        .resizable()
                        .aspectRatio(contentMode: .fit)
                        .frame(height: 32)
                        .padding(.top, 50)
                    
                    Spacer()
                    
                    HStack(spacing: 6) {
                        Image("num_\(timeComponents.h1)")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        Image("num_\(timeComponents.h2)")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        
                        Image("num_colon")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        
                        Image("num_\(timeComponents.m1)")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        Image("num_\(timeComponents.m2)")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        
                        Image("num_colon")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        
                        Image("num_\(timeComponents.s1)")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                        Image("num_\(timeComponents.s2)")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(height: 64)
                    }
                    
                    Spacer()
                    
                    if showControls {
                        HStack(spacing: 30) {
                            Button(action: {
                                if isRunning {
                                    pauseTimer()
                                } else {
                                    startTimer()
                                }
                            }) {
                                Image(isRunning ? "pause_button" : "play_button")
                                    .resizable()
                                    .aspectRatio(contentMode: .fit)
                                    .frame(width: 60, height: 60)
                            }
                            
                            Button(action: resetTimer) {
                                Image("reset_button")
                                    .resizable()
                                    .aspectRatio(contentMode: .fit)
                                    .frame(width: 60, height: 60)
                            }
                        }
                        .padding(.bottom, 50)
                    } else {
                        Color.clear
                            .frame(height: 110)
                    }
                }
            }
        }
        .ignoresSafeArea()
        .onTapGesture {
            withAnimation {
                showControls.toggle()
            }
        }
        .statusBar(hidden: true)
        .preferredColorScheme(.dark)
        .onDisappear {
            pauseTimer()
        }
    }
    
    private func startTimer() {
        isRunning = true
        timer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in
            timeElapsed += 1
        }
    }
    
    private func pauseTimer() {
        isRunning = false
        timer?.invalidate()
        timer = nil
    }
    
    private func resetTimer() {
        pauseTimer()
        timeElapsed = 0
    }
}

#Preview {
    ContentView()
}

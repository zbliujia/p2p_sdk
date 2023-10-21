//
//  LocalNetworkPrivacy.swift
//  example
//
//  Created by ZYB on 2023/10/21.
//

import Foundation
import UIKit

class LocalNetworkPrivacy : NSObject {
    let service: NetService

    var completion: ((Bool) -> Void)?
    var timer: Timer?
    var publishing = false
    
    override init() {
        service = .init(domain: "local.", type:"_http._tcp", name: "LocalNetworkPrivacy", port: 1100)
        super.init()
    }
    
    @objc
    func checkAccessState(completion: @escaping (Bool) -> Void) {
        self.completion = completion
        
        timer = .scheduledTimer(withTimeInterval: 2, repeats: true, block: { timer in
            guard UIApplication.shared.applicationState == .active else {
                return
            }
            
            if self.publishing {
                self.timer?.invalidate()
                self.completion?(false)
            }
            else {
                self.publishing = true
                self.service.delegate = self
                self.service.publish()
                
            }
        })
    }
    
    deinit {
        service.stop()
    }
}

extension LocalNetworkPrivacy : NetServiceDelegate {
    
    func netServiceDidPublish(_ sender: NetService) {
        timer?.invalidate()
        completion?(true)
    }
}

//
//  ViewController.m
//  example
//
//  Created by Wiki on 2021/2/19.
//

#import "ViewController.h"
#import "Cross.h"
#import "GCDWebServer.h"
#import "GCDWebServerDataResponse.h"
#import <NetworkExtension/NEVPNManager.h>
#import "example-Swift.h"

@interface ViewController () {
    GCDWebServer* _webServer;
}

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSURL *url = [NSURL URLWithString:@"https://www.baidu.com"];
    NSData *data = [NSData dataWithContentsOfURL:url];
    
    [[[LocalNetworkPrivacy alloc] init] checkAccessStateWithCompletion:^(BOOL) {
        
    }];
    
//    [[NEVPNManager sharedManager] loadFromPreferencesWithCompletionHandler:^(NSError * _Nullable error) {
//        
//    }];
//    
//    NSString* url = @"http://example.com?key2=value2&key3=value3&key1=VALUE1";
    Cross *cross = [[Cross alloc] init];
//    NSString*newUrl = [cross signatureUrl:url];
    [cross startServer];
//    NSLog(url);
//    NSLog(newUrl);
    // Do any additional setup after loading the view.
    
    // Create server
      _webServer = [[GCDWebServer alloc] init];
      
      // Add a handler to respond to GET requests on any URL
      [_webServer addDefaultHandlerForMethod:@"GET"
                                requestClass:[GCDWebServerRequest class]
                                processBlock:^GCDWebServerResponse *(GCDWebServerRequest* request) {
        
        return [GCDWebServerDataResponse responseWithHTML:@"<html><body><p>Hello World</p></body></html>"];
        
      }];
      
      // Start server on port 8080
      [_webServer startWithPort:18080 bonjourName:nil];
      NSLog(@"Visit %@ in your web browser", _webServer.serverURL);
    
}
@end

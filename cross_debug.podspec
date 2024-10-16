Pod::Spec.new do |s|
  s.name             = 'cross_debug'
  s.version          = '0.0.1'
  s.summary          = 'cross library'
  s.description      = 'Cross library'
  s.homepage         = 'http://example.com'
#   s.license          = { :file => '../LICENSE' }
  s.author           = { 'Your Company' => 'email@example.com' }
  # s.source           = { :path => '.' }
  s.ios.vendored_libraries = 'third_party/libhv/lib/Debug/libhv_static.a', 'third_party/libhv/lib/Debug/libcrypto.a', 'third_party/libhv/lib/Debug/libssl.a'
  s.source           = { :git => 'git@github.com:zbliujia/p2p_sdk.git', :tag => s.version.to_s }
  # 设置源文件，切记不要把测试代码包含进来
  s.source_files = 'ios/Classes/**/*','third_party/3rd/**/*.{c,cc,cpp,h,hpp}','third_party/libhv/**/*.{c,cc,cpp,h,hpp}','src/**/*.{cc,cpp,h,hpp}'
  # 暴露头文件，否则引用该spec的项目无法找到头文件
  s.public_header_files = 'ios/Classes/**/*.h'
  s.project_header_files = 'src/**/*.h'
  s.platform = :ios, '11.0'
  # 必须配置HEADER_SEARCH_PATHS属性，是否会导致项目中C++找不到头文件
  s.xcconfig = {
        'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/third_party/3rd/" "${PODS_TARGET_SRCROOT}/third_party/libhv/include/" '
  }
  s.pod_target_xcconfig = { 'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'arm64' }
  s.user_target_xcconfig = { 'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'arm64' }
  
end

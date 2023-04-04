myDir = File.dirname(File.expand_path(__FILE__));
require "#{myDir}/../build-options.rb";

module Rakish

Rakish::CppProject.new(
	:name 			=> "artd-jlib-net",
	:package 		=> "artd",
	:dependsUpon 	=> [ "../artd-lib-logger",
	                     "../artd-jlib-base",
	                     "../artd-jlib-thread",
	                     "../artd-jlib-io"
	                   ]
) do

	cppDefine('BUILDING_artd_jlib_net');
		
	addPublicIncludes('include/artd/*.h');

    addSourceFiles(
        './OsSocket.cpp',
        './ServerSocket.cpp',
        './SimpleSocketClient.cpp',
#        './SimpleSocketServer.cpp',
        './Socket.cpp',
        './SocketInputStream.cpp',
        './SocketOutputStream.cpp'
    );

    setupCppConfig :targetType =>'DLL' do |cfg|
    end
end

end # module Rakish


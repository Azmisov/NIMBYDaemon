//This is a really simple server for writing the NIMBY changes to file
var http = require('http');
var fs = require('fs');

http.createServer(function (req, res) {
	if (req.method == "POST"){
		console.log("Writing new NIMBY configuration");
		var new_json = "";
		req.on('data', function(data){
			new_json += data;
			if (new_json.length > 1e6)
				req.connection.destroy();
		});
		req.on('end', function(){
			fs.writeFileSync("NIMBYDaemon.config", new_json);
			finish_request(res, "Wrote new NIMBY configuration file");
		});
	}
	else finish_request(res, "");
}).listen(9615);
console.log("Server running on port 9615");

function finish_request(res, txt){
	res.writeHead(200, {
		'Content-Type': 'text/plain',
		'Access-Control-Allow-Origin': '*'
	});
	res.end(txt);
}

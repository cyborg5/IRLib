var api=pinoccioAPI();

//Enter your token here. Also troop ID and scout ID
api.token="enter your token here";
var troopid= 1;//replace this "1" with your troop ID
var scoutid=1;//replace this "1" with your scout ID

//handles to various parts of the screen. Initialized in myInitialize()
var LoginResults, RemoteErr, RemoteTable, KeyMessage;
var DisplaySymbols;
var myAccount= {};
var HotKeys= [];
var Button= [//object containing all buttons
	[//object containing row 0
		[7,0xe0e0e01f,0, "V+",187],//Object containing button 1
		[5,0x37c906,0, "|&#9668;&#9668;",74],//jump back "J"
		[5,0x37291a,0, "&#9668;&#9668;",36],//rewind
		[5,0x37990c,0, "&#9658;",32],//play
		[5,0x36293a,0, "&#9658;&#9658;",35],//fast-forward
		[5,0x36113d,0, "1",49],
		[5,0x37111d,0, "2",50],
		[5,0x36912d,0, "3",51]
	],
	[//row 1
		[7,0xe0e0d02f,0, "V-",189],
		[5,0x36b129,0, "&#9658;&#9658;|",78],//live "N"
		[5,0x375914,0, "<span class='error Big'>&#9679;</span>",82],//record "R"
		[5,0x374117,0, "||",80],
		[5,0x365934,0, "<span class='Big'>&#9632;</span>",83],//Stop
		[5,0x37910d,0, "4",52],
		[5,0x365135,0, "5",53],
		[5,0x375115,0, "6",54]
	],
	[//row 2
		[7,0xe0e0f00f,0, "Mute",81],
		[5,0x36c127,0, "Guide",71],
		[5,0x37d904,0, "PgDn", 34],
		[5,0x36812f,0, "<span class=' Big'>&#8657;</span>",38],//up arrow
		[5,0x36d924,0, "PgUp",33],
		[5,0x36d125,0, "7",55],
		[5,0x37d105,0, "8",56],
		[5,0x363139,0, "9",57]
	],
	[//row 3
		[5,0x377111,0, "Ch+",85],
		[5,0x366932,0, "Exit",88],
		[5,0x37810f,0, "<span class=' Big'>&#8656;</span>", 37],//left arrow
		[5,0x366133,0, "OK",13],
		[5,0x364137,0, "<span class=' Big'>&#8658;</span>",39],//right arrow
		[5,0x36213b,0, "Info",73],
		[5,0x373119,0, "0",48],
		[5,0x36b928,0, "Zoom",90]
	],
	[//row 4
		[5,0x36f121,0, "Ch-",68],
		[5,0x36e123,0, "Prev",86],
		[5,0x373918,0, "Menu",77],
		[5,0x37a10b,0, "<span class=' Big'>&#8659;</span>",40],//down arrow
		[5,0x36c926,0, "List",76],
		[5,0x37e902,0, "A",65],
		[5,0x36193c,0, "B",66],
		[5,0x37191c,0, "C",67]
	],
	[//row 5
		[5,0x37b908,0, "PIP",84],
		[5,0x377910,0, "PIP<br>Move",220],
		[5,0x367930,0, "PIP<br>Swap",191],
		[5,0x37f900,0, "PIP-",188],
		[5,0x36e922,0, "PIP+",190],
		[5,0x37f101,0, "Fav",70],
		[5,0x37c107,0, "Cbl<br>Pwr",0],
		[7,0x0e0e0f0bf,0, "TV<br>Pwr",0]
	],
	[//row 6
		[5,0x0e163d,0, "Day-",219],
		[5,0x0fe603,0, "Day+",221]
	]
/*
	[//row 
		[0,0x0,0, "",0],
		[0,0x0,0, "",0],
		[0,0x0,0, "",0],
		[0,0x0,0, "",0],
		[0,0x0,0, "",0]
	]
*/
]

function copy_properties (d,s) {for(x in s)d[x]=s[x];}
function myDisplayErr(handle,s,err) {
	handle.innerHTML= "<span class='error'>Error during "+s+err.code
					+" '"+err.message+"'</span>";
}

function myGetAccountInfo(verbose,cb){
	api.rest({url:"/v1/account"},
		function(err,data) {
			if(err) {
				if (verbose)myDisplayErr(LoginResults,"getting account info ",err);
			} else {
				api.account=data.id; 
				copy_properties(myAccount,data);
			}
			cb(err);
		}
	)
}
function myInitialize() {
	LoginResults=document.getElementById("LoginResults");
	RemoteErr=document.getElementById ("RemoteErr");
	RemoteTable=document.getElementById("RemoteTable");
	KeyMessage=document.getElementById("KeyMessage");
	CommandQueue=document.getElementById("CommandQueue");
	document.onkeydown= function(e){KeyHandler(e);};
	myGetAccountInfo(true,
		function(err) {
			if(err) {
				RemoteErr.innerHTML="Error with Token ='"+api.token+"' troop ID: "+troopid+
					"scout ID: "+ scoutid;
			} else {
				LoginResults.innerHTML= 
					"Welcome "+myAccount.firstname+" "+myAccount.lastname+
					"&nbsp; &nbsp; &nbsp; You are logged in '"+myAccount.email;
				SetupHotKeys ();
				DisplaySymbols= false;
				ShowRemote ();
			}
		}
	)
}
     
function KeyHandler (e) {
var x,r,c,s;
    e = e || window.event;
	if(e.which) x=e.which; else x=e.keyCode;
	if(x==27) {DisplaySymbols=!DisplaySymbols;ShowRemote(); return;}
	s = TranslateSpecial(x);
	var t="You pressed '"+s+"' ("+x+")";
	if(HotKeys[x][0]>=0)
		SendButton(t+" which corresponds to",HotKeys[x][0],HotKeys[x][1]);
	 else
		KeyMessage.innerHTML=t;
		//RemoteErr.innerHTML= "";
}

function TranslateSpecial(x) {
	switch(x) {
		case 187:s="+"; break; case 189:s="-"; break;
		case 36:s="HOME"; break; case 35:s="END"; break;
		case 38:s="UPAR"; break; case 40:s="DNAR"; break;
		case 37:s="LFAR"; break; case 39:s="RTAR"; break;
		case 33:s="PGUP"; break; case 34:s="PGDN"; break;
		case 13:s="ENTER"; break; case 32:s="SPACE"; break;
		case 219:s="["; break; case 221:s="]"; break;
		case 188:s=","; break; case 190:s="."; break;
		case 191:s="/"; break; case 120:s="|"; break;
		case 20:s="ESC"; break;
		default:s= String.fromCharCode(x);
	}
	return s;
}
function SetupHotKeys () {
	for(var k=0;k<256;k++) HotKeys[k]=[-1,-1];
}
function ShowRemote () {
	var r,c;
	RemoteTable.innerHTML= "";
	for(r=0;r< Button.length;r++) {
		var TR= document.createElement ("tr");
		var TD= "";
		for(var c=0;c<Button[r].length;c++) {
			TD=TD+"<td class='MyButtons' onclick='DoButton("+r+","+c+")'>"+
				(DisplaySymbols?TranslateSpecial(Button[r][c][4]):Button[r][c][3])
				+"</td>";
			if(Button[r][c][4]>0) HotKeys[Button[r][c][4]]=[r,c];
		}
		TR.innerHTML =TD;
		RemoteTable.appendChild(TR);
	}
}
function DoButton (r,c) {
	SendButton("You clicked",r,c);
}
var myQ= [];
function SendButton (t,r,c) {
	var B= Button[r][c];
	KeyMessage.innerHTML=t+" button("+r+","+c+")='"+B[3]+
		"'  Protocol:"+ B[0]+"  Code:0x"+B[1].toString(16)+
		"   Bits:"+B[2];
	myQ.unshift(B)
	IRsend();
}
function IRsend() {
	var t="";
	for(var i=0;i<myQ.length;i++) {
		t=t+"P:"+myQ[i][0]+" C: 0x"+myQ[i][1]+" B:"+myQ[i][2]+"<br>";
	}
	CommandQueue.innerHTML=t;
	if(myQ.length==0) return;
	var B=myQ.shift();
	if(B[0]==0) return;
	RemoteErr.innerText= "Attempting sending..."; 
	api.rest (
		{url:"/v1/"+troopid+"/"+scoutid+"/command", 
		data:{command:"ir.send("+B[0]+","+B[1]+","+B[2]+")"}
		},
		function (err,data) {
			var txt= "";
			if(err) {
				txt =err.code+ " message: "+err.message;
			} else {
				txt = "Sent:"+B[0]+ ", 0x"+B[1].toString(16)+ ", "+B[2];
			}
			RemoteErr.innerText=txt;
			IRsend();
		}
	)
}

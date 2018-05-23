// Quite closely based on sample code at https://wicg.github.io/webusb/

var device_labels = {};

function add_device_to_ui(device) {
    var label = device.productName + " " + device.serialNumber;
    if (!device_labels[label]) {
      var button = document.createElement("button");
      button.appendChild(document.createTextNode(label));
      button.addEventListener('click', function () { connect_device(device); });
      document.getElementById("devices").append(button);
      device_labels[label] = [device, button];
    }
}   
       
function remove_device_from_ui(device) {
    var label = device.productName + " " + device.serialNumber;
    if (device_labels[label]) {
      device_labels[label][1].remove();
      delete device_labels[label];
    }
}

document.addEventListener('DOMContentLoaded', async () => {
    let devices = await navigator.usb.getDevices();
    devices.forEach(device => add_device_to_ui(device));
});

navigator.usb.addEventListener('connect', ev => add_device_to_ui(ev.device));

navigator.usb.addEventListener('disconnect', ev => remove_device_from_ui(ev.device));

document.getElementById('add-device').addEventListener('click', async () => {
    let device;
    try {
      navigator.usb.requestDevice({ filters: [{ vendorId: 0x1209, productId: 0xadda }] }).then(
         device => add_device_to_ui(device)
      );
    } catch (err) {
      console.log(err);
    }
});

var keyboard = document.getElementById('keyboard');
var typewriter = document.getElementById('typewriter');
var stopper = document.getElementById('stopper');
var hammer = document.getElementById('hammer');
var decoder = new TextDecoder();
var encoder = new TextEncoder();

async function connect_device(device) {

    await device.open();
    await device.claimInterface(2);

    var connected = 1;

    keyboard.onkeypress = async function (ev) {
        if (ev.code == 'Enter') {
        	console.log(keyboard.value);
		        //let b1 = encoder.encode(keyboard.value + "\r\n");
			//let b2 = new Uint8Array(64);
			//b2.set(b1);
			//await device.transferOut(5, b2);
	await	device.transferOut(5, encoder.encode(keyboard.value + "\r\n"));
			keyboard.value = "";
		}
    }

     stopper.onclick = async function (ev) {
	     await device.transferOut(5, new Uint8Array([3]));
     }

     hammer.onclick = async function (ev) {
	     await device.transferOut(5, new Uint8Array([4]));
     }

	while(connected) {
  	    let result = await device.transferIn(4, 64);
  	    let message = new Uint8Array(result.data.buffer).filter(c => c != 0);
  	    if (message.byteLength > 0) {
		    typewriter.innerText += decoder.decode(message);
	    }
	}

}


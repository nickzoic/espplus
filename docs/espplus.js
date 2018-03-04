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

async function connect_device(device) {
    await device.open();
    let result = await device.controlTransferIn({ requestType: 'vendor', recipient: 'device', request: 1, value: 2, index: 3}, 6);
    console.log(result.data);
}
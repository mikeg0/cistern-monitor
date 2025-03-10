var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
    websocket.onerror = onError;
}
function onOpen(event) {
    document.getElementById('network-connection-lost').style.display = "none";
    console.log('Connection opened');

    // setTimeout(() => websocket.send(JSON.stringify({'eventType': 'init'})), 1000);
    websocket.send(JSON.stringify({'eventType': 'init'}))
}
function onClose(event) {
    console.log('Connection closed');
    document.getElementById('network-connection-lost').style.display = "block";

    setTimeout(initWebSocket, 2000);
}
function onError(event) {
    console.log('Connection error');
    console.log(event);

    document.getElementById('network-connection-lost').style.display = "block";

    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    let eventData = JSON.parse(event.data);

    switch (eventData.type) {
        case "REAL_TIME_STATS":
            document.getElementById('toggle-real-time').checked = eventData.value;
            break;

        case "SYSTEM_STATE":
            document.getElementById('water-level-state').innerHTML = eventData.value.waterLevel + "%";
            document.getElementById('ps-voltage').innerHTML = "(" + eventData.value.psVoltage + ")";
            document.getElementById('max-water-level').innerHTML = eventData.value.maxWaterLevel  + "%";
            document.getElementById('min-water-level').innerHTML = eventData.value.minWaterLevel + "%";
            document.getElementById('pump-current').innerHTML = eventData.value.pumpCurrent + " Amps";

            break;

        case "HIGH_WATER_ALARM":
            if (eventData.value) {

                document.title = "Overflow Alarm!";

                document.getElementById('high-water-state').innerHTML = "ON";
                document.getElementById('high-water-state').classList.add("alarm");
                document.getElementById('high-water-state-card').classList.add("alarm");
            }
            else {
                document.getElementById('high-water-state').innerHTML = "OFF";
                document.getElementById('high-water-state').classList.remove("alarm");
                document.getElementById('high-water-state-card').classList.remove("alarm");
            }
            break;

        case "LOW_WATER_ALARM":
            if (eventData.value) {

                document.title = "Low Water Alarm!";
                document.getElementById('low-water-state').innerHTML = "ON";
                document.getElementById('low-water-state').classList.add("alarm");
                document.getElementById('low-water-state-card').classList.add("alarm");
            }
            else {
                document.getElementById('low-water-state').innerHTML = "OFF";
                document.getElementById('low-water-state').classList.remove("alarm");
                document.getElementById('low-water-state-card').classList.remove("alarm");
            }

            // document.getElementById('reset-low-water-alarm-button').disabled = false;
            break;

    }
}
function onLoad(event) {
    setTimeout(initWebSocket, 1000);
    initButtons();
}
function initButtons() {
//    document.getElementById('reset-low-water-alarm-button').addEventListener('click', () => websocket.send({'eventType': 'reset'}));

    document.getElementById('reset-water-level-minmax').addEventListener('click', () => {
        websocket.send(JSON.stringify({'eventType': 'reset'}))
    });

    document.getElementById('toggle-real-time').addEventListener('click', () => {
        websocket.send(JSON.stringify({'eventType': 'real-time', 'value': (document.getElementById('toggle-real-time').checked) ? 1 : 0}))
    });


    document.getElementById('send-status-message').addEventListener('click', () => {
        websocket.send(JSON.stringify({'eventType': 'status-message', 'statusMessage': document.getElementById('status-message').value}))
    });


    document.getElementById('status-message').addEventListener('keyup', (event) => {
        // add keyup event listener for carraige return 13 to send status message
        if (event.code === "Enter") {

            // Prevent the default action
            event.preventDefault();

            console.log(event.target.value)

            websocket.send(JSON.stringify({'eventType': 'status-message', 'statusMessage': document.getElementById('status-message').value}))
            document.getElementById('status-message').value = ''
          }
    });

}

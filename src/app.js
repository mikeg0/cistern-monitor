var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}
function onOpen(event) {
    console.log('Connection opened');
}
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}
var minWaterLevel = 0;
var maxWaterLevel = 0;
function onMessage(event) {
    let eventData = JSON.parse(event.data);

    switch (eventData.type) {
        case "WATER_LEVEL":
            document.getElementById('water-level-state').innerHTML = eventData.value + " mm";

            if (minWaterLevel == 0 || minWaterLevel > eventData.value) minWaterLevel = eventData.value

            if (maxWaterLevel < eventData.value) maxWaterLevel = eventData.value

            document.getElementById('max-water-level').value = maxWaterLevel + " mm";
            document.getElementById('min-water-level').value = minWaterLevel + " mm";

            break;

        case "HIGH_WATER_ALARM":
            // TODO: send browser notification

            if (eventData.value) {
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
            document.getElementById('low-water-state').innerHTML = (eventData.value == 1) ? "ON" : "OFF";
            document.getElementById('reset-low-water-alarm-button').disabled = false;
            break;

    }
}
function onLoad(event) {
    initWebSocket();
    initResetButton();
}
function initResetButton() {
    document.getElementById('reset-low-water-alarm-button').addEventListener('click', () => websocket.send('reset'));
}

import QtQuick 2.7
import QtQuick.Controls 1.4
import OpenGL 1.0

ApplicationWindow {
    visible: true
    width: 800
    height: 480
    color: "black"

    Squircle {
        SequentialAnimation on t {
            NumberAnimation { to: 1; duration: 7500; easing.type: Easing.Bezier }
            NumberAnimation { to: 0; duration: 7500; easing.type: Easing.Bezier }
            loops: Animation.Infinite
            running: true
        }
    }

    Text {
        font.family: "Arial";
        font.pointSize: 11;
        color: "white"
        text: "The background here is a squircle rendered with raw OpenGL using the 'beforeRender()' signal in QQuickWindow."
        anchors.bottom: parent.bottom
        anchors.margins: 5
    }
}

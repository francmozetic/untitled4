import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import Qt.labs.folderlistmodel 2.1
import Audio 1.0

ApplicationWindow {
    visible: true
    width: 800
    height: 480
    color: "white"

    StackLayout {
        id:layout1
        x: 9
        y: 9
        width: 782
        height: 350
        currentIndex: 0

        Flickable {
            id: flickable1
            width: 782
            height: 350
            contentWidth: levels1.width
            contentHeight: levels1.height
            clip: true

            PaintedLevels { // Here is a QML component with a signal named qmlSignal that is emitted with a string-type parameter.
                id: levels1
                objectName: "levels1"
                width: 2500
                height: 350
                signal qmlSignal(string msg)
                signal pathSignal(string msg)

                MouseArea {
                    id: mouseArea1
                    anchors.fill: parent
                    hoverEnabled: true
                    onPressed: levels1.qmlSignal(mouse.x)
                    onClicked: levels1.paintClicked(mouse.x);
                }
            }
        }

        Rectangle {
            x: 9
            y: 9
            width: 782
            height: 350
            color: "black"

            ListModel {
                id: listModel1

                ListElement {
                    name: "audio1.wav"
                    path: "@"
                    size: "0"
                }
                ListElement {
                    name: "audio2.wav"
                    path: "@"
                    size: "0"
                }
                ListElement {
                    name: "audio3.wav"
                    path: "@"
                    size: "0"
                }
                ListElement {
                    name: "audio4.wav"
                    path: "@"
                    size: "0"
                }
                ListElement {
                    name: "audio5.wav"
                    path: "@"
                    size: "0"
                }
            }

            FolderListModel {
                id: folderModel1
                nameFilters: ["*.wav"]
            }

            Component {
                id: header1
                Row {
                    Text { text: 'name'; width: 250; font.family: "Arial"; font.pointSize: 25; color: "steelblue" }
                    Text { text: 'path'; width: 370; font.family: "Arial"; font.pointSize: 25; color: "steelblue" }
                    Text { text: 'size'; width: 150; font.family: "Arial"; font.pointSize: 25; color: "steelblue" }
                }
            }

            Component {
                id: listDelegate1
                Row {
                    Text { text: name; width: 250; font.family: "Arial"; font.pointSize: 25; color: "white"

                        MouseArea {
                            id: mouseArea1
                            anchors.fill: parent
                            onClicked: listView1.currentIndex = index
                        }
                    }
                    Text { text: path; width: 370; font.family: "Arial"; font.pointSize: 25; color: "white"

                        MouseArea {
                            id: mouseArea2
                            anchors.fill: parent
                            onClicked: listView1.currentIndex = index
                        }
                    }
                    Text { text: path; width: 150; font.family: "Arial"; font.pointSize: 25; color: "white"

                        MouseArea {
                            id: mouseArea3
                            anchors.fill: parent
                            onClicked: listView1.currentIndex = index
                        }
                    }
                }
            }

            Component {
                id: fileDelegate1
                Row {
                    Text { text: fileName; width: 250; font.family: "Arial"; font.pointSize: 25; color: "white"

                        MouseArea {
                            id: mouseArea1
                            anchors.fill: parent
                            onClicked: { listView1.currentIndex = index; levels1.pathSignal(filePath); }
                        }
                    }
                    Text { text: filePath; width: 370; font.family: "Arial"; font.pointSize: 25; color: "white"

                        MouseArea {
                            id: mouseArea2
                            anchors.fill: parent
                            onClicked: { listView1.currentIndex = index; levels1.pathSignal(filePath); }
                        }
                    }
                    Text { text: fileSize; width: 150; font.family: "Arial"; font.pointSize: 25; color: "white"

                        MouseArea {
                            id: mouseArea3
                            anchors.fill: parent
                            onClicked: { listView1.currentIndex = index; levels1.pathSignal(filePath); }
                        }
                    }
                }
            }

            ListView {
                id: listView1
                anchors.fill: parent
                header: header1
                model: folderModel1
                delegate: fileDelegate1
                highlight: Rectangle {
                    color: "steelblue"
                    width: parent.width
                    y: listView1.currentItem.y
                    Behavior on y {
                        SpringAnimation {
                            spring: 1
                            damping: 0.2
                        }
                    }
                }
                currentIndex: 0
            }
        }
    }

    Button {
        id: button1
        x: 9
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            fillMode: Image.Stretch
            source: "ic_mic_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { flickable1.contentX=0; flickable1.interactive=true; levels1.levelsRecord(); }
    }

    Button {
        id: button2
        x: 115
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            source: "ic_play_arrow_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { flickable1.contentX=0; flickable1.interactive=false; levels1.levelsPlay(); }
    }

    Button {
        id: button3
        x: 221
        y: 371
        width: 100
        height: 100
        Image {
            source: "ic_content_cut_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { flickable1.contentX=0; flickable1.interactive=true; levels1.levelsWaveform(); }
    }

    Button {
        id: button4
        x: 327
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            source: "ic_fingerprint_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { flickable1.contentX=0;  flickable1.interactive=true; levels1.levelsFingerprint(); }
    }

    Button {
        id: button5
        x: 433
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            source: "ic_cloud_queue_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { flickable1.contentX=0; flickable1.interactive=true; levels1.levelsCloudQueue(); }
    }

    Button {
        id: button6
        x: 538
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            source: "ic_face_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { flickable1.contentX=0; flickable1.interactive=true; levels1.levelsFace(); }
    }

    Button {
        id: button7
        x: 691
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            source: "ic_gesture_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { layout1.currentIndex=1; button1.visible=false; button2.visible=false; button3.visible=false; button4.visible=false;
            button5.visible=false; button6.visible=false; button7.visible=false; button10.visible=true; }
    }

    Button {
        id: button10
        x: 691
        y: 371
        width: 100
        height: 100
        text: qsTr("")
        Image {
            source: "ic_gesture_black_36dp.png"
            anchors.centerIn: parent
        }
        onClicked: { layout1.currentIndex=0; button1.visible=true; button2.visible=true; button3.visible=true; button4.visible=true;
            button5.visible=true; button6.visible=true; button7.visible=true; button10.visible=false; }
    }
}

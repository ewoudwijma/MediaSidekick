import QtQuick 2.0
import QtQuick.Window 2.0
import QtLocation 5.6
import QtPositioning 5.6

//https://books.google.nl/books?id=hvBZDwAAQBAJ&pg=PA190&lpg=PA190&dq=QGeoServiceProvider+.pro&source=bl&ots=rPcVXaaZ3Q&sig=ACfU3U1SkQ8B0QIUESliep_cZAnPAtaieg&hl=nl&sa=X&ved=2ahUKEwivv8708OrmAhUSUlAKHcf4D1UQ6AEwAXoECAoQAQ#v=onepage&q=QGeoServiceProvider%20.pro&f=false

Item {
    width: 512
    height: 350
    visible: true
//    anchors.fill: parent

    id: window

    Plugin {
        id: mapPlugin
        name: "esri" // "mapboxgl", "esri", ...osm
        // specify plugin parameters if necessary
        // PluginParameter {
        //     name:
        //     value:
        // }
        PluginParameter
                {
                    name: "osm.mapping.host"
                    value: "https://a.tile.openstreetmap.org"
                }
        PluginParameter { name: "osm.useragent"; value: "My great Qt OSM application" }
//            PluginParameter { name: "osm.mapping.host"; value: "http://osm.tile.server.address/" }
            PluginParameter { name: "osm.mapping.copyright"; value: "All mine" }
//            PluginParameter { name: "osm.routing.host"; value: "http://osrm.server.address/viaroute" }
//            PluginParameter { name: "osm.geocoding.host"; value: "http://geocoding.server.address" }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
//        center: QtPositioning.coordinate(59.91, 10.75) // Oslo
        zoomLevel: 14

//        Component.onCompleted:
//        {
//            addMarker("ewoud", 59.91, 10.75)
//        }
    }

    function clearMapItems()
    {
        map.clearMapItems()
    }

    function addMarker(name, latitude, longitude)
    {
        var component = Qt.createComponent("qrc:/amarker.qml")
        var item = component.createObject(window, {coordinate: QtPositioning.coordinate(latitude, longitude), labelText:name})
        map.addMapItem(item)
    }

    function center(latitude, longitude)
    {
        map.center = QtPositioning.coordinate(latitude, longitude)
    }

    function fitViewportToMapItems()
    {
        map.fitViewportToMapItems()
        if (map.zoomLevel > 14)
            map.zoomLevel = 14
    }
}

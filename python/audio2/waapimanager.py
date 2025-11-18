"""Handles all communication with Wwise Authoring Tool via WAAPI."""

try:
    import audio2
except ImportError:
    audio2 = None


class WaapiManager:
    """Manages WAAPI connection and operations for audio editing tools."""

    def __init__(self):
        self._waapi_manager = None
        self._is_connected = False
        self._subscribed_attenuation_id = None
        self._subscription_callback = None

    @property
    def isConnected(self):
        """Returns whether currently connected to Wwise Authoring."""
        return self._is_connected

    def _getWaapiManagerInstance(self):
        """Gets or creates the WAAPI manager instance from audio2."""
        if not audio2:
            return None
        if not self._waapi_manager:
            try:
                self._waapi_manager = audio2.WaapiManager()
            except AttributeError:
                return None
        return self._waapi_manager

    def toggleConnection(self):
        """Toggle connection to Wwise Authoring Tool."""
        manager = self._getWaapiManagerInstance()
        if not manager:
            return False

        try:
            if self._is_connected:
                # Unsubscribe before disconnecting
                self.unsubscribeFromMaxRadius()
                manager.Disconnect()
                self._is_connected = False
            else:
                success = manager.Connect("127.0.0.1", 8080)
                is_connected = manager.IsConnected()
                self._is_connected = success and is_connected

            return self._is_connected

        except Exception:
            self._is_connected = False
            return False

    def loadSoundsForEvent(self, event_name):
        """Loads sounds referenced by the given event name."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected:
            return []

        event_variations = [
            event_name,
            event_name.lower(),
            event_name.upper(),
            "\\Events\\{}".format(event_name),
            "\\Events\\Default Work Unit\\{}".format(event_name),
        ]

        target_names = []
        target_ids = []

        for event_variant in event_variations:
            try:
                target_names = manager.GetEventReferencedTargetNames(event_variant)
                target_ids = manager.GetEventReferencedTargetIds(event_variant)
                if target_names:
                    break
            except Exception:
                continue

        if target_names:
            options = [
                {
                    "displayName": name,
                    "soundId": target_ids[i] if i < len(target_ids) else "",
                    "attenuations": []
                }
                for i, name in enumerate(target_names)
            ]
            return options

        return []

    def loadAttenuationsForSound(self, sound_id):
        """Load attenuations for the given sound ID."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected or not sound_id:
            return []

        try:
            attenuation_id = manager.GetSoundReferencedAttenuationId(sound_id)
            attenuation_name = manager.GetSoundReferencedAttenuationName(sound_id)

            if not attenuation_id:
                return []

            # Get max radius
            try:
                max_radius = manager.GetAttenuationMaxRadius(attenuation_id)
            except Exception:
                max_radius = 0.0
            else:
                if max_radius is None:
                    max_radius = 0.0

            # Get attenuation name if not already retrieved
            if not attenuation_name:
                attenuation_name = manager.GetAttenuationName(attenuation_id)

            # Get volume curves
            volume_curve_data = None
            distances = manager.GetAttenuationVolumeCurveDistances(attenuation_id)
            volumes = manager.GetAttenuationVolumeCurveValues(attenuation_id)
            shapes = manager.GetAttenuationVolumeCurveShapeInts(attenuation_id)

            if distances and volumes and shapes:
                volume_curve_data = {
                    'distances': list(distances),
                    'volumes': list(volumes),
                    'shapes': list(shapes)
                }

            return [{
                "displayName": attenuation_name or "Attenuation ({})".format(attenuation_id),
                "name": attenuation_id,
                "maxRadius": max_radius,
                "volumeCurveData": volume_curve_data,
            }]

        except Exception:
            return []

    def getMaxRadiusFromWwise(self, attenuation_id):
        """Gets the current max radius value from Wwise for the given attenuation."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected or not attenuation_id:
            return 0.0

        try:
            max_radius = manager.GetAttenuationMaxRadius(attenuation_id)
            if max_radius is None:
                max_radius = 0.0
            return max_radius
        except Exception:
            return 0.0

    def updateMaxRadiusInWwise(self, attenuation_id, value):
        """Updates max radius in Wwise for the given attenuation."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected or not attenuation_id:
            return value

        try:
            success = manager.SetAttenuationMaxRadius(attenuation_id, value)

            # Refresh to get actual value from Wwise
            refreshed_radius = manager.GetAttenuationMaxRadius(attenuation_id)
            if refreshed_radius is None:
                refreshed_radius = 0.0
            return refreshed_radius

        except Exception:
            return value

    def updateVolumeCurveInWwise(self, attenuation_id, curve_data, max_radius):
        """Updates volume curve in Wwise for the given attenuation."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected or not attenuation_id or not curve_data:
            return False

        try:
            distances = curve_data.get('distances', [])
            volumes = curve_data.get('volumes', [])
            shapes = curve_data.get('shapes', [])

            # Validate that curve points don't exceed max radius
            if self._validateCurveData(distances, max_radius):
                return False

            success = manager.SetAttenuationVolumeCurve(attenuation_id, distances, volumes, shapes)
            return success

        except Exception:
            return False

    def _validateCurveData(self, distances, max_radius):
        """Validates that all distances are within max radius."""
        if not distances or max_radius <= 0:
            return False
        return any(distance > max_radius for distance in distances)

    def subscribeToMaxRadius(self, attenuation_id, callback):
        """Subscribe to max radius changes for the given attenuation."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected or not attenuation_id:
            return False

        # Unsubscribe from previous attenuation if any
        if self._subscribed_attenuation_id:
            self.unsubscribeFromMaxRadius()

        try:
            # Store the user's callback
            self._subscription_callback = callback

            # Subscribe via C++ WAAPI manager
            success = manager.SubscribeToAttenuationMaxRadius(attenuation_id, self._onMaxRadiusChanged)

            if success:
                self._subscribed_attenuation_id = attenuation_id

            return success

        except Exception:
            return False

    def unsubscribeFromMaxRadius(self):
        """Unsubscribe from max radius changes for the currently subscribed attenuation."""
        if not self._subscribed_attenuation_id:
            return

        manager = self._getWaapiManagerInstance()
        if not manager:
            return

        try:
            manager.UnsubscribeFromPropertyChanges(self._subscribed_attenuation_id)
        finally:
            self._subscribed_attenuation_id = None
            self._subscription_callback = None

    def _onMaxRadiusChanged(self, object_id, property_name):
        """Internal callback invoked when MaxRadius changes in Wwise."""
        # Invoke the user's callback if registered
        if self._subscription_callback:
            self._subscription_callback(object_id, property_name)

    def createAttenuation(self, attenuation_name, parent_path):
        """Creates a new attenuation in Wwise.

        Args:
            attenuation_name: Name for the new attenuation
            parent_path: Path or ID of parent (e.g., "\\Attenuations\\Default Work Unit")

        Returns:
            The created attenuation ID if successful, None otherwise
        """
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected:
            return None

        try:
            # Call the overloaded function that returns the ID directly
            attenuation_id = manager.CreateAttenuation(attenuation_name, parent_path)
            return attenuation_id if attenuation_id else None

        except Exception:
            return None

    def setReference(self, object_id, reference_name, reference_id):
        """Sets a reference property on a Wwise object.
        Args:
            object_id: ID of the object to modify
            reference_name: Name of the reference property (e.g., "Attenuation", "OutputBus")
            reference_id: ID of the object to reference (empty string to clear reference)

        Returns:
            True if successful, False otherwise
        """
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected:
            return False

        try:
            return manager.SetReference(object_id, reference_name, reference_id)
        except Exception:
            return False

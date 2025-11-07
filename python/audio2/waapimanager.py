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

        except Exception as e:
            print(f"Exception during WAAPI connection toggle: {e}")
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
            f"\\Events\\{event_name}",
            f"\\Events\\Default Work Unit\\{event_name}",
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
                try:
                    attenuation_name = manager.GetAttenuationName(attenuation_id)
                except Exception:
                    pass

            # Get volume curves
            volume_curve_data = None
            try:
                distances = manager.GetAttenuationVolumeCurveDistances(attenuation_id)
                volumes = manager.GetAttenuationVolumeCurveValues(attenuation_id)
                shapes = manager.GetAttenuationVolumeCurveShapeInts(attenuation_id)

                if distances and volumes and shapes:
                    volume_curve_data = {
                        'distances': list(distances),
                        'volumes': list(volumes),
                        'shapes': list(shapes)
                    }
            except Exception:
                pass

            return [{
                "displayName": attenuation_name or f"Attenuation ({attenuation_id})",
                "name": attenuation_id,
                "maxRadius": max_radius,
                "volumeCurveData": volume_curve_data,
            }]

        except Exception as e:
            print(f"Exception loading attenuations for sound: {e}")
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
        except Exception as e:
            print(f"Exception getting max radius from Wwise: {e}")
            return 0.0

    def updateMaxRadiusInWwise(self, attenuation_id, value):
        """Updates max radius in Wwise for the given attenuation."""
        manager = self._getWaapiManagerInstance()
        if not manager or not self._is_connected or not attenuation_id:
            return value

        try:
            success = manager.SetAttenuationMaxRadius(attenuation_id, value)

            # Refresh to get actual value from Wwise
            refreshed_radius = None
            try:
                refreshed_radius = manager.GetAttenuationMaxRadius(attenuation_id)
            except Exception:
                pass

            if refreshed_radius is None:
                refreshed_radius = 0.0

            return refreshed_radius

        except Exception as e:
            print(f"Exception updating max radius in Wwise: {e}")
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
                print("Curve validation failed: distances exceed max radius")
                return False

            success = manager.SetAttenuationVolumeCurve(attenuation_id, distances, volumes, shapes)
            return success

        except Exception as e:
            print(f"Exception updating volume curve in Wwise: {e}")
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
            print("Cannot subscribe: Not connected or no attenuation ID")
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
                print(f"Successfully subscribed to MaxRadius changes for attenuation: {attenuation_id}")
                return True
            else:
                print(f"Failed to subscribe to MaxRadius changes for attenuation: {attenuation_id}")
                return False

        except Exception as e:
            print(f"Exception subscribing to MaxRadius changes: {e}")
            return False

    def unsubscribeFromMaxRadius(self):
        """Unsubscribe from max radius changes for the currently subscribed attenuation."""
        if not self._subscribed_attenuation_id:
            return

        manager = self._getWaapiManagerInstance()
        if not manager:
            return

        try:
            success = manager.UnsubscribeFromPropertyChanges(self._subscribed_attenuation_id)
            if success:
                print(f"Successfully unsubscribed from attenuation: {self._subscribed_attenuation_id}")
            else:
                print(f"Failed to unsubscribe from attenuation: {self._subscribed_attenuation_id}")

        except Exception as e:
            print(f"Exception unsubscribing from MaxRadius changes: {e}")

        finally:
            self._subscribed_attenuation_id = None
            self._subscription_callback = None

    def _onMaxRadiusChanged(self, object_id, property_name):
        """Internal callback invoked when MaxRadius changes in Wwise."""
        print(f"WAAPI notification: {property_name} changed for object {object_id}")

        # Invoke the user's callback if registered
        if self._subscription_callback:
            try:
                self._subscription_callback(object_id, property_name)
            except Exception as e:
                print(f"Exception in user callback: {e}")

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

            if attenuation_id:
                return attenuation_id
            else:
                return None

        except Exception as e:
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
            print("Cannot set reference: Not connected to Wwise")
            return False

        try:
            success = manager.SetReference(object_id, reference_name, reference_id)

            if success:
                if reference_id:
                    print(f"Successfully set reference '{reference_name}' on object {object_id}")
                else:
                    print(f"Successfully cleared reference '{reference_name}' on object {object_id}")
            else:
                print(f"Failed to set reference '{reference_name}' on object {object_id}")

            return success

        except Exception as e:
            print(f"Exception setting reference: {e}")
            return False

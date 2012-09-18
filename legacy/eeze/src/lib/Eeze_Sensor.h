#ifndef EEZE_SENSOR_H
#define EEZE_SENSOR_H

#ifdef EAPI
# undef EAPI
#endif

#ifdef __GNUC__
# if __GNUC__ >= 4
#  define EAPI __attribute__ ((visibility("default")))
# else
#  define EAPI
# endif
#else
# define EAPI
#endif

#include <Eina.h>

/**
 * @file Eeze_Sensor.h
 * @brief Sensor information
 * @since 1.8
 *
 * Eeze sensor functions allow you to gather sensor information from different sensor sources
 * available on the hardware. It supports a plugin architecture to support different hardware
 * platforms and devices.
 *
 * @addtogroup sensor Sensor
 * @{
 */

/**
 * @enum Eeze_Sensor_Type
 * @since 1.8
 *
 * All sensor types known by Eeze Sensor. This list of types include real physical types like
 * proximity or light as well as "aggregated" types like putting a device down on the dsiplay side
 * (facedown).
 */
typedef enum
{
   EEZE_SENSOR_TYPE_ACCELEROMETER, /**< Accelerometer sensor */
   EEZE_SENSOR_TYPE_MAGNETIC, /**< Magnetic sensor */
   EEZE_SENSOR_TYPE_ORIENTATION, /**< Orientation sensor */
   EEZE_SENSOR_TYPE_GYROSCOPE, /**< Gyroscope sensor */
   EEZE_SENSOR_TYPE_LIGHT, /**< Light sensor */
   EEZE_SENSOR_TYPE_PROXIMITY, /**< Proximity sensor */
   EEZE_SENSOR_TYPE_MOTION_SNAP, /**< Snap motion sensor */
   EEZE_SENSOR_TYPE_MOTION_SHAKE, /**< Shake motion sensor */
   EEZE_SENSOR_TYPE_MOTION_DOUBLETAP, /**< Doubletap motion sensor */
   EEZE_SENSOR_TYPE_MOTION_PANNING, /**< Panning motion sensor */
   EEZE_SENSOR_TYPE_MOTION_FACEDOWN, /**< Facedown motion sensor */
   /* Non-Tizen from here */
   EEZE_SENSOR_TYPE_BAROMETER, /**< Barometer sensor */
   EEZE_SENSOR_TYPE_TEMPERATURE, /**< Temperature sensor */
   EEZE_SENSOR_TYPE_LAST = 0xFF /**< Last item to mark end of enum */
}  Eeze_Sensor_Type;

/**
 * @defgroup Sensor_Events Available eeze sensor events
 * @brief Sensor events that are emitted from the library as ecore events
 *
 * Event types used to regoister ecore_event_handler on. These events are used for
 * #eeze_sensor_async_read to deliver read out data. It is also used for generated events like
 * facedown or shake.
 * @since 1.8
 * @{
 */
EAPI int EEZE_SENSOR_EVENT_ACCELEROMETER;
EAPI int EEZE_SENSOR_EVENT_MAGNETIC;
EAPI int EEZE_SENSOR_EVENT_ORIENTATION;
EAPI int EEZE_SENSOR_EVENT_GYROSCOPE;
EAPI int EEZE_SENSOR_EVENT_LIGHT;
EAPI int EEZE_SENSOR_EVENT_PROXIMITY;
EAPI int EEZE_SENSOR_EVENT_SNAP;
EAPI int EEZE_SENSOR_EVENT_SHAKE;
EAPI int EEZE_SENSOR_EVENT_DOUBLETAP;
EAPI int EEZE_SENSOR_EVENT_PANNING;
EAPI int EEZE_SENSOR_EVENT_FACEDOWN;
EAPI int EEZE_SENSOR_EVENT_BAROMETER;
EAPI int EEZE_SENSOR_EVENT_TEMPERATURE;
/**@}*/

/**
 * @typedef Eeze_Sensor
 * @since 1.8
 *
 * Handle for an Eeze_Sensor.
 */
typedef struct _Eeze_Sensor
{
   Eina_Array *modules_array;  /**< Array of available runtime modules */
   Eina_Hash  *modules; /**< Hash with loaded modules */
}  Eeze_Sensor;

/**
 * @typedef Eeze_Sensor_Obj;
 * @since 1.8
 *
 * Object for a sensor type. Keeps information about the type and holds the data for the accessor
 * functions. As this information gets also updated by async reads it might be a good idea to check
 * the timestamp value to see when the data has been updated. The timestamp is given as unix epoch
 * (seconds since 00:00:00 UTC on 1 January 1970).
 */
typedef struct _Eeze_Sensor_Obj
{
   unsigned int   type; /**< Sensor type see #Eeze_Sensor_Type */
   int   accuracy; /**< Accuracy of the sensor value */
   float data[3]; /**< Sensor data depending on the sensor type */
   unsigned long long timestamp; /**< Timestamp of data read */
   Eina_Bool continuous_flow; /**< FUTURE USE: Continuous flow of sensor read out */
} Eeze_Sensor_Obj;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create sensor object to operate on.
 * @param type Sensor type to create object from.
 * @return Sensor object for the given type.
 *
 * Takes the sensor type and create an object for it to operate on. During this it also does an
 * initial sensor data read to fill the sensor data into the object.
 * The #eeze_sensor_free function must be used to destroy the object and release its memory.
 * @since 1.8
 */
EAPI Eeze_Sensor_Obj *eeze_sensor_new(Eeze_Sensor_Type type);

/**
 * @brief Free a sensor object.
 * @param sens Sensor object to operate on.
 *
 * Free an sensor object when it is no longer needed.
 * @since 1.8
 */
EAPI void eeze_sensor_free(Eeze_Sensor_Obj *sens);

/**
 * @brief Get accuracy from sensor object.
 * @param sens Sensor object to operate on.
 * @param accuracy Pointer to write accurancy value into.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * Accessor function to get the accurancy property from the sensor object.
 * @since 1.8
 */
EAPI Eina_Bool eeze_sensor_accuracy_get(Eeze_Sensor_Obj *sens, int *accuracy);

/**
 * @brief Get data from all three data properties
 * @param sens Sensor object to operate on.
 * @param x Pointer to write first data property value into.
 * @param y Pointer to write second data property value into.
 * @param z Pointer to write third data property value into.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * Accessor function to get all three data properties from the sensor object. This is used for sensor
 * types that offer all three values. Like acceleromter and magnetic.
 * @since 1.8
 */
EAPI Eina_Bool eeze_sensor_xyz_get(Eeze_Sensor_Obj *sens, float *x, float *y, float *z);

/**
 * @brief Get data from first two data properties
 * @param sens Sensor object to operate on.
 * @param x Pointer to write first data property value into.
 * @param y Pointer to write second data property value into.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * Accessor function to get the first two data properties from the sensor object. This is used for sensor
 * types that offer two values. Like panning.
 * @since 1.8
 */
EAPI Eina_Bool eeze_sensor_xy_get(Eeze_Sensor_Obj *sens, float *x, float *y);

/**
 * @brief Get the data from  first data property
 * @param sens Sensor object to operate on.
 * @param x Pointer to write first data property value into.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * Accessor function to get the first data property from the sensor object. This is used for sensor
 * types that only offer one value. Like light or proximity.
 * @since 1.8
 */
EAPI Eina_Bool eeze_sensor_x_get(Eeze_Sensor_Obj *sens, float *x);

/**
 * @brief Get timestamp from sensor object.
 * @param sens Sensor object to operate on.
 * @param timestamp Pointer to write timestamp value into.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * Accessor function to get the timestamp property from the sensor object.
 * @since 1.8
 */
EAPI Eina_Bool eeze_sensor_timestamp_get(Eeze_Sensor_Obj *sens, unsigned long long *timestamp);

/**
 * @brief Read out sensor data
 * @param sens Sensor object to operate on.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * This function reads sensor data from the device and fills the sensor object with the data. This
 * call is synchronuos and blocks until the data is read out and updated in the sensor object.
 * For simple applications this is fine and the easiest way to use the API.
 * @since 1.8
 */
EAPI Eina_Bool eeze_sensor_read(Eeze_Sensor_Obj *sens);

/**
 * @brief Asynchronous read out sensor data
 * @param sens Sensor object to operate on.
 * @param user_data Data to pass to the callback function.
 * @return EINA_TRUE for success and EINA_FALSE for failure
 *
 * This function reads sensor data from the device and fills the sensor object with the data. The
 * read is done asynchronously and thus does not block after calling. Instead the given callback
 * function is called once the read is finished and the object filled.
 * @since 1.8
 */
EAPI Eina_Bool       eeze_sensor_async_read(Eeze_Sensor_Obj *sens, void *user_data);

/**
 * @brief Helper function to access the sensor handle
 * @return The sensor handle to operate on
 *
 * @since 1.8
 */
EAPI Eeze_Sensor *eeze_sensor_handle_get(void);

/**
 * @brief Fetch the sensor object by type from the sensor object list
 * @param type Sensor type to fetch from the list of sensor objects.
 * @return The sensor object matching the given type
 *
 * @since 1.8
 */
EAPI Eeze_Sensor_Obj    *eeze_sensor_obj_get(Eeze_Sensor_Type type);

Eina_Bool            eeze_sensor_init(void);
void                 eeze_sensor_shutdown(void);

#ifdef __cplusplus
}
#endif
/** @} */
#endif

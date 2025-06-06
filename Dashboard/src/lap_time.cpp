#include "dashboard.h"

bool getIntersectionTime(GPSPoint prev, GPSPoint curr)
{
  if (!doIntersect(prev, curr))
    return false;

  double x1 = prev.lon, y1 = prev.lat;
  double x2 = curr.lon, y2 = curr.lat;
  double x3 = gateL.lon, y3 = gateL.lat;
  double x4 = gateR.lon, y4 = gateR.lat;

  double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
  if (denom == 0)
    return false;

  double interLon = ((x1 * y2 - y1 * x2) * (x3 - x4) -
                     (x1 - x2) * (x3 * y4 - y3 * x4)) /
                    denom;
  double interLat = ((x1 * y2 - y1 * x2) * (y3 - y4) -
                     (y1 - y2) * (x3 * y4 - y3 * x4)) /
                    denom;

  double distToInter = hypot(interLon - prev.lon, interLat - prev.lat);
  double avgSpeed = (prev.speed + curr.speed) / 2.0;
  if (avgSpeed < 1)
    avgSpeed = 1; // avoid division by 0

  double ratio = distToInter / (avgSpeed / 3.6); // km/h → m/s

  currTime = prev.timestamp + ratio * (curr.timestamp - prev.timestamp);
  return true;
}

int orientation(GPSPoint p, GPSPoint q, GPSPoint r)
{
    double val = (q.lon - p.lon) * (r.lat - q.lat) -
                 (q.lat - p.lat) * (r.lon - q.lon);
    if (val == 0.0)
        return 0;               // colinear
    return (val > 0.0) ? 1 : 2; // clock or counterclockwise
}

bool onSegment(GPSPoint p, GPSPoint q, GPSPoint r)
{
    return q.lon <= fmax(p.lon, r.lon) && q.lon >= fmin(p.lon, r.lon) &&
           q.lat <= fmax(p.lat, r.lat) && q.lat >= fmin(p.lat, r.lat);
}

bool doIntersect(GPSPoint p1, GPSPoint q1)
{
  int o1 = orientation(p1, q1, gateL);
  int o2 = orientation(p1, q1, gateR);
  int o3 = orientation(gateL, gateR, p1);
  int o4 = orientation(gateL, gateR, q1);

  if (o1 != o2 && o3 != o4)
    return true;
  if (o1 == 0 && onSegment(p1, gateL, q1))
    return true;
  if (o2 == 0 && onSegment(p1, gateR, q1))
    return true;
  if (o3 == 0 && onSegment(gateL, p1, gateR))
    return true;
  if (o4 == 0 && onSegment(gateL, q1, gateR))
    return true;

  return false;
}
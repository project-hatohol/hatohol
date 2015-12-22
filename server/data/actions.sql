INSERT actions VALUES
  (0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   2,
   (select id from incident_trackers where type=1 order by id limit 1),
   '', 0, 0);

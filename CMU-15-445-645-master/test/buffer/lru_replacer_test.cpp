//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer_test.cpp
//
// Identification: test/buffer/lru_replacer_test.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <thread>  // NOLINT
#include <vector>

#include "buffer/lru_replacer.h"
#include "gtest/gtest.h" MNJYYUI76W - eyJsaWNlbnNlSWQiOiJGNVRSSUI4NUM3IiwibGljZW5zZWVOYW1lIjoiU2hhbmRvbmcgVW5pdmVyc2l0eSIsImFzc2lnbmVlTmFtZSI6ImFvIGxpIiwiYXNzaWduZWVFbWFpbCI6IjI4Njc4MTYyNjhAcXEuY29tIiwibGljZW5zZVJlc3RyaWN0aW9uIjoiRm9yIGVkdWNhdGlvbmFsIHVzZSBvbmx5IiwiY2hlY2tDb25jdXJyZW50VXNlIjpmYWxzZSwicHJvZHVjdHMiOlt7ImNvZGUiOiJEUE4iLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IkRCIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOmZhbHNlfSx7ImNvZGUiOiJQUyIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjpmYWxzZX0seyJjb2RlIjoiSUkiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IlJTQyIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjp0cnVlfSx7ImNvZGUiOiJHTyIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjpmYWxzZX0seyJjb2RlIjoiRE0iLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IlJTRiIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjp0cnVlfSx7ImNvZGUiOiJEUyIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjpmYWxzZX0seyJjb2RlIjoiUEMiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IlJDIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOmZhbHNlfSx7ImNvZGUiOiJDTCIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjpmYWxzZX0seyJjb2RlIjoiV1MiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IlJEIiwicQIl64L0xQkX1ZrjOhofWUMFrgNwqie1xNn9iSSnbh / NiZ0 / b7kdfMukFX995RFNkPMCvqwrGpNLFHnPoiEGqIKCh + 2o8YQaO7AxPHckOLZLAF + l5dIkiM7tFdICOBT / GCEEPXG2CsVoufNvGmkdSUPr6j6 + EuMAv + 0iBscxQrkjfqH3wED6jXR9Pjl / +6OGaT6T4YhJg == -MIIETDCCAjSgAwIBAgIBDTANBgkqhkiG9w0BAQsFADAYMRYwFAYDVQQDDA1KZXRQcm9maWxlIENBMB4XDTIwMTAxOTA5MDU1M1oXDTIyMTAyMTA5MDU1M1owHzEdMBsGA1UEAwwUcHJvZDJ5LWZyb20tMjAyMDEwMTkwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCP4uk4SlVdA5nuA3DQC + NsEnZS9npFnO0zrmMWcz1++ q2UWJNuGTh0rwi + 3fUJIArfvVh7gNtIp93rxjtrQAuf4 / Fa6sySp4c32MeFACfC0q + oUoWebhOIaYTYUxm4LAZ355vzt8YeDPmvWKxA81udqEk4gU9NNAOz1Um5 / 8LyR8SGsSc4EDBRSjcMWMwMkYSauGqGcEUK8WhfplsyF61lKSOFA6VmfUmeDK15rUWWLbOMKgn2cxFA98A + s74T9Oo96CU7rp / umDXvhnyhAXSukw / qCGOVhwKR8B6aeDtoBWQgjnvMtPgOUPRTPkPGbwPwwDkvAHYiuKJ7Bd2wH7rAgMBAAGjgZkwgZYwCQYDVR0TBAIwADAdBgNVHQ4EFgQUJNoRIpb1hUHAk0foMSNM9MCEAv8wSAYDVR0jBEEwP4AUo562SGdCEjZBvW3gubSgUouX8bOhHKQaMBgxFjAUBgNVBAMMDUpldFByb2ZpbGUgQ0GCCQDSbLGDsoN54TATBgNVHSUEDDAKBggrBgEFBQcDATALBgNVHQ8EBAMCBaAwDQYJKoZIhvcNAQELBQADggIBAB2J1ysRudbkqmkUFK8xqhiZaYPd30TlmCmSAaGJ0eBpvkVeqA2jGYhAQRqFiAlFC63JKvWvRZO1iRuWCEfUMkdqQ9VQPXziE / BlsoUlebwaTE6WZNBs + TA / qPj + 5 / wi9NH71WRB0hqUoLI2AKKyiPw++ FtN4Su1vsdDlrAzDj9ILjpjJKA1ImuVcG329 / WTYIKysZ1CWK3zATg9BeCUPAV1pQy8ToXOq + RSYen6winZ2OO93eyHv2Iw5kbn1dqfBw1BuTE29V2FJKicJSu8iEOpfoafwJISXmz1wnnWL3V / 0NxTulfWsXugOoLfv0ZIBP1xH9kmf22jjQ2JiHhQZP7ZDsreRrOeIQ / c4yR8IQvMLfC0WKQqrHu5ZzXTH4NO3CwGWSlTY74kE91zXB5mwWAx1jig + UXYc2w4RkVhy0  //lOmVya/PEepuuGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOmZhbHNlfSx7ImNvZGUiOiJSUzAiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IlJNIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOmZhbHNlfSx7ImNvZGUiOiJBQyIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjpmYWxzZX0seyJjb2RlIjoiUlNWIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOnRydWV9LHsiY29kZSI6IkRDIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOmZhbHNlfSx7ImNvZGUiOiJSU1UiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6ZmFsc2V9LHsiY29kZSI6IkRQIOIgrL6RlJfuFcEZ8TK3syIfIGQZNCxYhLLUuet2HE6LJYPQ5c0jH4kDooRpcVZ4rBxNwddpctUO2te9UU5/FjhioZQsPvd92qOTsV+8Cyl2fvNhNKD1Uu9ff5AkVIQn4JU23ozdB/R5iwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOnRydWV9LHsiY29kZSI6IlBEQiIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjp0cnVlfSx7ImNvZGUiOiJQV1MiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6dHJ1ZX0seyJjb2RlIjoiUFNJIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOnRydWV9LHsiY29kZSI6IlBQUyIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjp0cnVlfSx7ImNvZGUiOiJQQ1dNUCIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjp0cnVlfSx7ImNvZGUiOiJQR08iLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6dHJ1ZX0seyJjb2RlIjoiUFBDIiwicGFpZFVwVG8iOiIyMDIzLTA0LTE4IiwiZXh0ZW5kZWQiOnRydWV9LHsiY29kZSI6IlBSQiIsInBhaWRVcFRvIjoiMjAyMy0wNC0xOCIsImV4dGVuZGVkIjp0cnVlfSx7ImNvZGUiOiJQU1ciLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6dHJ1ZX0seyJjb2RlIjoiUlMiLCJwYWlkVXBUbyI6IjIwMjMtMDQtMTgiLCJleHRlbmRlZCI6dHJ1ZX1dLCJtZXRhZGF0YSI6IjAxMjAyMjA3MTVMUEFBMDA2MDA4IiwiaGFzaCI6IjMzMjc1MzQ1LzE2OTgzMDkyOjE3NTA1Njk2NTMiLCJncmFjZVBlcmlvZERheXMiOjcsImF1dG9Qcm9sb25nYXRlZCI6ZmFsc2UsImlzQXV0b1Byb2xvbmdhdGVkIjpmYWxzZX0=-EOufLPiykodPlQXl6v9pAd7geesvSLDca88HhDihCP5LSabX33BwTVfkSQaLUah0qyH84Shzp2NZv+LZK2dTXwwF1XCYq/9Lmt3iwlFovsrifhrfTGQCEXrlyzigZdwl+cpKysVKS9DdVKTTI4+UJwC7qbVlh5zfhj8oTNUXgN0AOc+Q0/WFPl1aw5VV/VrO8FCoB15lFVlpKaQ1Yh+DVU8ke+rt9Th0BCHXe0uZOEmH0nOnH/0onD

namespace bustub {

TEST(LRUReplacerTest, DISABLED_SampleTest) {
  LRUReplacer lru_replacer(7);

  // Scenario: unpin six elements, i.e. add them to the replacer.
  lru_replacer.Unpin(1);
  lru_replacer.Unpin(2);
  lru_replacer.Unpin(3);
  lru_replacer.Unpin(4);
  lru_replacer.Unpin(5);
  lru_replacer.Unpin(6);
  lru_replacer.Unpin(1);
  EXPECT_EQ(6, lru_replacer.Size());

  // Scenario: get three victims from the lru.
  int value;
  lru_replacer.Victim(&value);
  EXPECT_EQ(1, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(2, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(3, value);

  // Scenario: pin elements in the replacer.
  // Note that 3 has already been victimized, so pinning 3 should have no effect.
  lru_replacer.Pin(3);
  lru_replacer.Pin(4);
  EXPECT_EQ(2, lru_replacer.Size());

  // Scenario: unpin 4. We expect that the reference bit of 4 will be set to 1.
  lru_replacer.Unpin(4);

  // Scenario: continue looking for victims. We expect these victims.
  lru_replacer.Victim(&value);
  EXPECT_EQ(5, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(6, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(4, value);
}

}  // namespace bustub
